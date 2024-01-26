#!/usr/bin/env python3
# openssl.py


import os, subprocess, sys
from tools import helpers



def compile_openssl(env):

	target_platform = env["platform"]
#	arch = env["arch"]
#	bits = "64" if arch in ["x86_64", "arm64", "rv64", "ppc64"] else "32"
#	architecture = get_architecture(env)
	win_host = is_win_host(env)

	jobs = str(env.GetOption("num_jobs"))


	cmd_config = ["perl", "Configure"] if win_host else ["./Configure"]
	cmd_depend = ["nmake", "test"] if win_host else ["make", "depend"]
	cmd_compile = ['nmake'] if win_host else ["make", "-j" + jobs]
	cmd_install =  ["nmake install"] if win_host else ["make", "install"]




	target = get_target(env)
	cmd_config.append(target)

	options = ssl_options(target_platform)
	cmd_config.extend(options)

	cross_compilation_param = get_cross_compilation_param(env)
	if cross_compilation_param != "":
		cmd_config.append(cross_compilation_param)

	openssl_path = get_openssl_path(env)
	lib_path = get_openssl_install_path(env)
	cmd_config.append("--prefix=" + lib_path)
	cmd_config.append("--openssldir=" + lib_path)
	cmd_config.append("-Wl,-rpath=" + lib_path + "/lib64")

	if not os.path.exists(lib_path):
		os.makedirs(lib_path)
	
	subprocess.check_call(cmd_config, cwd=openssl_path, env={"PATH": f"{openssl_path}:{os.environ['PATH']}"})
	subprocess.check_call(cmd_depend, cwd=openssl_path, env={"PATH": f"{openssl_path}:{os.environ['PATH']}"})
	subprocess.check_call(cmd_compile, cwd=openssl_path, env={"PATH": f"{openssl_path}:{os.environ['PATH']}"})
	subprocess.check_call(cmd_install, cwd=openssl_path, env={"PATH": f"{openssl_path}:{os.environ['PATH']}"})

	return 0






def is_win_host(env):
	return sys.platform in ["win32", "msys", "cygwin"]


def get_cross_compilation_param(env):

	platform = env["platform"]
	host = helpers.get_host()
	target_bits = "64" if env["arch"] in ["x86_64", "arm64", "rv64", "ppc64"] else "32"
	host_bits =  helpers.get_host_bits()
	is_cross_compile = (host != platform or host_bits != target_bits)

	if not is_cross_compile:
		return ""

	if platform == "windows":
		if not (is_win_host(env) or env.get("is_msvc", False)):
			if target_bits == "64":
				return "--cross-compile-prefix=x86_64-w64-mingw32-"
			else:
				return "--cross-compile-prefix=i686-w64-mingw32-"

	elif platform == "linuxbsd":
		if target_bits == "64":
			return "--cross-compile-prefix=x86_64-linux-gnu-"
		else:
			return"--cross-compile-prefix=i686-linux-gnu-"

	return ""



def get_openssl_install_path(env):
	_lib_path = [os.getcwd(), "3party", "bin", env["platform"], env["arch"]]
	if env["use_llvm"]:
		_lib_path.append("llvm")
	_lib_path.append("openssl")
	lib_path = ""
	if is_win_host(env):
		lib_path = "\\-=-".join(_lib_path)
		lib_path = lib_path.replace("-=-", "")
	else:
		lib_path = "/".join(_lib_path)
	return lib_path


def get_openssl_path(env):
	_openssl_path = [os.getcwd(), "3party", "openssl"]
	openssl_path = ""
	if is_win_host(env):
		openssl_path = "\\-=-".join(_openssl_path)
		openssl_path = openssl_path.replace("-=-", "")
	else:
		openssl_path = "/".join(_openssl_path)
	return openssl_path




def get_target(env):

	platform = env["platform"]
	arch = env["arch"]
	compiller = helpers.get_compiller(env)

	if platform == "linuxbsd":
		if compiller == "gcc":
			if arch == "x86_32":
				return "linux-x86"
			elif arch == "x86_64":
				return "linux-x86_64"
			elif arch == "arm32":
				return "linux-armv4"
			elif arch == "arm64":
				return "linux-aarch64"
			elif arch == "rv64":
				return "linux64-riscv64"
			elif arch == "ppc32":
				return "linux-ppc"
			elif arch == "ppc64":
				return "linux-ppc64"
			elif arch == "ppc64":
				return "linux-ppc64"

		elif compiller == "clang":
			if arch == "x86_32":
				return "linux-x86-clang"
			elif arch == "x86_64":
				return "linux-x86_64-clang"
			elif arch == "arm32":
				return "linux-armv4"
			elif arch == "arm64":
				return "linux-aarch64"
			elif arch == "rv64":
				return "linux64-riscv64"
			elif arch == "ppc32":
				return "linux-ppc"
			elif arch == "ppc64":
				return "linux-ppc64"
			elif arch == "ppc64":
				return "linux-ppc64"

	elif platform == "windows":
		if env.msvc:
			if arch == "x86_32":
				return "VC-WIN32"
			else:
				return  "VC-WIN64A"
		else:
			if arch == "x86_32":
				return "mingw"
			else:
				return  "mingw64"



# https://wiki.openssl.org/index.php/Compilation_and_Installation#Supported_Platforms

	print("FALHA:" + arch + " -> " + platform)
	raise ValueError("Architecture '%s' not supported for platform: '%s'" % (arch, platform))
	return ""



def get_architecture(env):
	architecture = ""
	if env["arch"] in ["x86_32", "x86_64"]:
		return "x86"
	elif env["arch"] in ["arm32", "arm64"]:
		return "arm"
	elif env["arch"] in ["rv64"]:
		return "riscv"
	elif env["arch"] in ["ppc32", "ppc64"]:
		return "power"
	elif env["arch"] in ["wasm32"]:
		return "wasm"

	raise ValueError("Architecture '%s' not supported for platform: '%s'" % (env["arch"], env["platform"]))
	return architecture




# https://wiki.openssl.org/index.php/Compilation_and_Installation
# Olhar antes de adicionar novas plataformas
def ssl_options(platform):
	ssl_config_options = [
		"no-ssl3",
		"no-weak-ssl-ciphers",
		"no-legacy",
		"no-shared",
		"shared",
		"no-tests",
	]

	if platform == "windows":
		ssl_config_options.append("enable-capieng")

	return ssl_config_options




def update_openssl():
	git_cmd = ["git pull", "--recurse-submodules"]
	subprocess.run(git_cmd, shell=True, cwd="3party/openssl")

