#!/usr/bin/env python3
# openssl.py


import os, subprocess, sys
from tools import helpers



def compile_openssl(env):

	is_win_host = sys.platform in ["win32", "msys", "cygwin"]
#	variant = "release" if env["target"] == "template_release" else "debug"
	platform = env["platform"]
#	arch = env["arch"]
#	bits = "64" if arch in ["x86_64", "arm64", "rv64", "ppc64"] else "32"
#	architecture = get_architecture(env)




	cmd_config = []
	if is_win_host:
		cmd_config.extend(["perl", "Configure"])
	else:
		cmd_config.append("./Configure")

	target = get_target(env)
	cmd_config.append(target)

	options = ssl_options(platform)
	cmd_config.extend(options)

	cross_compilation_param = get_cross_compilation_param(env)
	if cross_compilation_param != "":
		cmd_config.append(cross_compilation_param)


	openssl_path = get_openssl_path(env)
	lib_path = get_boost_install_path(env)
	cmd_config.append("--prefix=" + lib_path)
	cmd_config.append("--openssldir=" + lib_path)
	cmd_config.append("-Wl,-rpath=" + lib_path + "/lib64")

	

	jobs = "-j" + str(env.GetOption("num_jobs"))

	cmd_compile = ["make", jobs]
	
	print(cmd_config)
	print()

	if not os.path.exists(lib_path):
		os.makedirs(lib_path)

	cmd_install = ["make", "install"]

	try:
		subprocess.check_call(cmd_config, cwd=openssl_path, env={"PATH": f"{openssl_path}:{os.environ['PATH']}"})
		subprocess.check_call(["make", "depend"], cwd=openssl_path, env={"PATH": f"{openssl_path}:{os.environ['PATH']}"})
		subprocess.check_call(cmd_compile, cwd=openssl_path, env={"PATH": f"{openssl_path}:{os.environ['PATH']}"})
		subprocess.check_call(cmd_install, cwd=openssl_path, env={"PATH": f"{openssl_path}:{os.environ['PATH']}"})
	except subprocess.CalledProcessError as e:
		print(f"Error trying to configure OpenSSL!: {e}")
		exit(1)



#	os.chdir(openssl_path)
#	subprocess.run(cmd_compile)
#	subprocess.run(["make", "install"], cwd=openssl_path)
#	subprocess.run(["git", "clean", "-f", "-X", "-q"], cwd=openssl_path)


	return 0



def get_cross_compilation_param(env):

	platform = env["platform"]
	host = helpers.get_host()
	target_bits = "64" if env["arch"] in ["x86_64", "arm64", "rv64", "ppc64"] else "32"
	host_bits =  helpers.get_host_bits()
	is_cross_compile = (host != platform or host_bits != target_bits)
	is_win_host = sys.platform in ["win32", "msys", "cygwin"]

	print("host_bits: ========", str(host_bits))

	if not is_cross_compile:
		return ""

	if platform == "windows":
		if not (is_win_host or env.get("is_msvc", False)):
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



def get_boost_install_path(env):
	bin_path = f"{os.getcwd()}/3party/bin"
	_lib_path = [bin_path, env["platform"], env["arch"]]
	if env["use_llvm"]:
		_lib_path.append("llvm")
	_lib_path.append("openssl")
	lib_path = "/".join(_lib_path)
	return lib_path


def get_openssl_path(env):
	openssl_path = f"{os.getcwd()}/3party/openssl"
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
		if env.get("is_msvc", False):
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

