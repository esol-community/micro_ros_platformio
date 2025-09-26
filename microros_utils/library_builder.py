import os, sys
import yaml
import shutil
import subprocess

from .utils import run_cmd
from .repositories import Repository, Sources

class CMakeToolchain:
    def __init__(self, path, cc, cxx, ar, cflags, cxxflags, cpppath):
        cmake_toolchain = """include(CMakeForceCompiler)
set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_CROSSCOMPILING 1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

SET (CMAKE_C_COMPILER_WORKS 1)
SET (CMAKE_CXX_COMPILER_WORKS 1)

set(CMAKE_C_COMPILER {C_COMPILER})
set(CMAKE_CXX_COMPILER {CXX_COMPILER})
set(CMAKE_AR {AR_COMPILER})

set(CMAKE_C_FLAGS_INIT "{C_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_INIT "{CXX_FLAGS}" CACHE STRING "" FORCE)

set(PLATFORMIO_CPPPATH {CPPPATH})

set(__BIG_ENDIAN__ 0)"""

        cmake_toolchain = cmake_toolchain.format(C_COMPILER=cc, 
                                                 CXX_COMPILER=cxx, 
                                                 AR_COMPILER=ar, 
                                                 C_FLAGS=cflags, 
                                                 CXX_FLAGS=cxxflags, 
                                                 CPPPATH=cpppath)

        with open(path, "w") as file:
            file.write(cmake_toolchain)

        self.path = os.path.realpath(file.name)

class Build:
    def __init__(self, library_folder, packages_folder, distro, python_env, microros_rmw_impl, mcu_system, rmw_transport):
        self.library_folder = library_folder
        self.packages_folder = packages_folder
        self.build_folder = library_folder + "/build"
        self.distro = distro
        self.microros_rmw_impl = microros_rmw_impl
        self.mcu_system = mcu_system
        self.rmw_transport = rmw_transport

        self.dev_packages = []
        self.mcu_packages = []

        self.dev_folder = self.build_folder + '/dev'
        self.dev_src_folder = self.dev_folder + '/src'
        self.mcu_folder = self.build_folder + '/mcu'
        self.mcu_src_folder = self.mcu_folder + '/src'

        self.library_path = library_folder + '/libmicroros'
        self.library = self.library_path + "/libmicroros.a"
        self.includes = self.library_path+ '/include'
        self.library_name = "microros"
        self.python_env = python_env
        self.env = {}

    def run(self, meta, toolchain, user_meta = ""):
        if os.path.exists(self.library):
            print("micro-ROS already built")
            return

        self.check_env()
        self.download_dev_environment()
        self.build_dev_environment()
        self.download_mcu_environment()
        self.build_mcu_environment(meta, toolchain, user_meta)
        self.package_mcu_library()

    def ignore_package(self, name):
        for p in self.mcu_packages:
            if p.name == name:
                p.ignore()

    def check_env(self):
        ROS_DISTRO = os.getenv('ROS_DISTRO')

        if (ROS_DISTRO):
            PATH = os.getenv('PATH')
            os.environ['PATH'] = PATH.replace('/opt/ros/{}/bin:'.format(ROS_DISTRO), '')
            os.environ.pop('AMENT_PREFIX_PATH', None)

        RMW_IMPLEMENTATION = os.getenv('RMW_IMPLEMENTATION')

        if (RMW_IMPLEMENTATION):
            os.environ['RMW_IMPLEMENTATION'] = self.microros_rmw_impl

        self.env = os.environ.copy()

    def download_dev_environment(self):
        os.makedirs(self.dev_src_folder, exist_ok=True)
        print("Downloading micro-ROS dev dependencies")
        for repo in Sources.dev_environments[self.distro]:
            repo.clone(self.dev_src_folder)
            print("\t - Downloaded {}".format(repo.name))
            self.dev_packages.extend(repo.get_packages())

    def build_dev_environment(self):
        print("Building micro-ROS dev dependencies")
        
        # Fix build: Ignore rmw_test_fixture_implementation in rolling
        touch_command = ''
        if self.distro in ('rolling', 'kilted'):
            touch_command = 'touch src/ament_cmake_ros/rmw_test_fixture_implementation/COLCON_IGNORE && '
        
        command = "cd {} && {} . {} && colcon build --cmake-args -DBUILD_TESTING=OFF -DPython3_EXECUTABLE=`which python`".format(self.dev_folder, touch_command, self.python_env)
        result = run_cmd(command, env=self.env)

        if 0 != result.returncode:
            print("Build dev micro-ROS environment failed: \n {}".format(result.stderr.decode("utf-8")))
            sys.exit(1)

    def download_mcu_environment(self):
        os.makedirs(self.mcu_src_folder, exist_ok=True)
        print("Downloading micro-ROS library")
        for repo in Sources.mcu_environments[self.distro]:
            repo.clone(self.mcu_src_folder)
            self.mcu_packages.extend(repo.get_packages())
            for package in repo.get_packages():
                if package.name in Sources.ignore_packages[self.distro] or package.name.endswith("_cpp"):
                    package.ignore()

                print('\t - Downloaded {}{}'.format(package.name, " (ignored)" if package.ignored else ""))

        self.download_extra_packages()

    def download_extra_packages(self):
        if not os.path.exists(self.packages_folder):
            print("\t - Extra packages folder not found, skipping...")
            return

        print("Checking extra packages")

        # Load and clone repositories from extra_packages.repos file
        extra_repos = self.get_repositories_from_yaml("{}/extra_packages.repos".format(self.packages_folder))
        for repo_name in extra_repos:
            repo_values = extra_repos[repo_name]
            version = repo_values['version'] if 'version' in repo_values else None
            Repository(repo_name, repo_values['url'], self.distro, version).clone(self.mcu_src_folder)
            print("\t - Downloaded {}".format(repo_name))

        extra_folders = os.listdir(self.packages_folder)
        if 'extra_packages.repos' in extra_folders:
            extra_folders.remove('extra_packages.repos')

        for folder in extra_folders:
            print("\t - Adding {}".format(folder))

        shutil.copytree(self.packages_folder, self.mcu_src_folder, ignore=shutil.ignore_patterns('extra_packages.repos'), dirs_exist_ok=True)

    def get_repositories_from_yaml(self, yaml_file):
        repos = {}
        try:
            with open(yaml_file, 'r') as repos_file:
                root = yaml.safe_load(repos_file)
                repositories = root['repositories']

            if repositories:
                for path in repositories:
                    repo = {}
                    attributes = repositories[path]
                    try:
                        repo['type'] = attributes['type']
                        repo['url'] = attributes['url']
                        if 'version' in attributes:
                            repo['version'] = attributes['version']
                    except KeyError as e:
                        continue
                    repos[path] = repo
        except (yaml.YAMLError, KeyError, TypeError) as e:
            print("Error on {}: {}".format(yaml_file, e))
        finally:
            return repos

    def get_mcu_options(self):
        mcu_options = " --no-warn-unused-cli "

        match self.mcu_system:
            case "ZEPHYR":
                mcu_options += "-DWITH_ZEPHYR=ON "
            case "FREERTOS_PLUS_TCP":
                mcu_options += "-DWITH_FREERTOS_PLUS_TCP=ON "
            case "ARDUINO_ESP32":
                mcu_options += "-DWITH_ARDUINO_ESP32=ON "
            case "ARDUINO_OPENCR":
                mcu_options += "-DWITH_ESPIDF=ON "
            case "WITH_MBED":
                mcu_options += "-DWITH_MBED=ON "

        if self.microros_rmw_impl == "rmw_zenoh_pico":
            if self.rmw_transport == "unicast":
                mcu_options += "-DRMW_ZENOH_PICO_TRANSPORT_TYPE=unicast "
            elif self.rmw_transport == "multicast":
                mcu_options += "-DRMW_ZENOH_PICO_TRANSPORT_TYPE=mcast "
            else:
                mcu_options += "-DRMW_ZENOH_PICO_TRANSPORT_TYPE=serial "

        return mcu_options

    def build_mcu_environment(self, meta_file, toolchain_file, user_meta = ""):
        print("Building micro-ROS library")

        mcu_cmake_options = self.get_mcu_options()

        # Exclude unused RWM implementations from the build target.
        touch_rmw_command = ''
        if self.microros_rmw_impl == "rmw_zenoh_pico":
            # Change the zenoh_pico and rmw_zenoh_pico codes and Cmake files for the micro_ros_platformio environment.
            # patch -uprN [src] < patch_file.patch
            result = subprocess.run(['git', 'apply', '--directory=build/mcu/src/zenoh-pico', './001_zenoh-pico.patch'], capture_output=True, text=True)
            if result.returncode != 0:
               print("001_zenoh-pico.patch failed:", result.stderr)

            # Do not build rmw-microxrcedds
            touch_rmw_command = 'touch src/rmw-microxrcedds/COLCON_IGNORE && touch src/Micro-XRCE-DDS-Client/COLCON_IGNORE && rm -f src/zenoh-pico/COLCON_IGNORE && rm -f src/rmw_zenoh_pico/COLCON_IGNORE && '
        else:
            # Do not build rmw_zenoh_pico
            touch_rmw_command = 'touch src/rmw_zenoh_pico/COLCON_IGNORE && touch src/zenoh-pico/COLCON_IGNORE && rm -f src/Micro-XRCE-DDS-Client/COLCON_IGNORE && rm -f src/rmw-microxrcedds/COLCON_IGNORE && '

        common_meta_path = self.library_folder + '/metas/common.meta'
        colcon_command = '. {} && colcon build --merge-install --packages-ignore-regex=.*_cpp --metas {} {} {} --cmake-args -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=OFF  -DTHIRDPARTY=ON  -DBUILD_SHARED_LIBS=OFF  -DBUILD_TESTING=OFF  -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE={} {} -DPython3_EXECUTABLE=`which python`'.format(self.python_env, common_meta_path, meta_file, user_meta, toolchain_file, mcu_cmake_options)
        command = "cd {} && . {}/install/setup.sh && {} {}".format(self.mcu_folder, self.dev_folder, touch_rmw_command, colcon_command)
        result = run_cmd(command, env=self.env)

        if 0 != result.returncode:
            print("Build mcu micro-ROS environment failed: \n{}".format(result.stderr.decode("utf-8")))
            sys.exit(1)

    def package_mcu_library(self):
        binutils_path = self.resolve_binutils_path()
        aux_folder = self.build_folder + "/aux"

        shutil.rmtree(aux_folder, ignore_errors=True)
        shutil.rmtree(self.library_path, ignore_errors=True)
        os.makedirs(aux_folder, exist_ok=True)
        os.makedirs(self.library_path, exist_ok=True)
        for root, dirs, files in os.walk(self.mcu_folder + "/install/lib"):
            for f in files:
                if f.endswith('.a'):
                    os.makedirs(aux_folder + "/naming", exist_ok=True)
                    os.chdir(aux_folder + "/naming")
                    if(f == "libzenohpico.a"):
                        # libzenohpico.a should not be extracted because some object file names are duplicated.
                        os.system("cp {} ../{}".format(root+"/"+f, f))
                    else:
                        os.system("{}ar x {}".format(binutils_path, root + "/" + f))
                        for obj in [x for x in os.listdir() if x.endswith('obj')]:
                            os.rename(obj, '../' + f.split('.')[0] + "__" + obj)

        os.chdir(aux_folder)
        command = "{binutils}ar rc libmicroros.a $(ls *.o *.obj 2> /dev/null); rm *.o *.obj 2> /dev/null; {binutils}ranlib libmicroros.a".format(binutils=binutils_path)
        result = run_cmd(command)

        # To combine libmicroros.a and libzenohpico.a 
        current_directory = os.getcwd()
        if(os.path.isfile("{}/libzenohpico.a".format(current_directory))):
            command = "{binutils}ar cqT lib_temp.a libmicroros.a libzenohpico.a;".format(binutils=binutils_path)
            result = run_cmd(command)

            p1 = subprocess.Popen(["echo", "-n", "-e", "'create lib_temp.a\naddlib libmicroros.a \naddlib libzenohpico.a\nsave\nend'"], stdout=subprocess.PIPE)
            p2 = subprocess.Popen(["{binutils}ar".format(binutils=binutils_path), "-M"], stdin=p1.stdout, stdout=subprocess.PIPE)
            p1.stdout.close()
            p2.returncode
            p2.wait()

            command = "{binutils}ranlib lib_temp.a".format(binutils=binutils_path)
            result = run_cmd(command)
            
            command = "rm libmicroros.a; mv lib_temp.a libmicroros.a".format(binutils=binutils_path)
            result = run_cmd(command)

        if 0 != result.returncode:
            print("micro-ROS static library build failed3: \n{}".format(result.stderr.decode("utf-8")))
            sys.exit(1)

        os.rename('libmicroros.a', self.library)

        # Copy includes
        shutil.copytree(self.build_folder + "/mcu/install/include", self.includes)

        # Fix include paths
        include_folders = os.listdir(self.includes)

        for folder in include_folders:
            folder_path = self.includes + "/{}".format(folder)
            repeated_path = folder_path + "/{}".format(folder)

            if os.path.exists(repeated_path):
                shutil.copytree(repeated_path, folder_path, copy_function=shutil.move, dirs_exist_ok=True)
                shutil.rmtree(repeated_path)

    def resolve_binutils_path(self):
        if sys.platform == "darwin":
            homebrew_binutils_path = "/opt/homebrew/opt/binutils/bin/"
            if os.path.exists(homebrew_binutils_path):
                return homebrew_binutils_path

            print("ERROR: GNU binutils not found. ({}) Please install binutils with homebrew: brew install binutils"
                  .format(homebrew_binutils_path))
            sys.exit(1)

        return ""
