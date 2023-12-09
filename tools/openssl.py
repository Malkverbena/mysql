#!/usr/bin/env python3
# openssl.py


import os, subprocess, sys



# https://wiki.openssl.org/index.php/Compilation_and_Installation
# Olhar antes de adicionar novas plataformas
def ssl_platform_options(env):
	ssl_config_options = [
		"no-ssl2",
		"no-ssl3",
		"no-weak-ssl-ciphers",
		"no-legacy",
		"no-shared",
		"no-tests",
	]

	if debug:
		ssl_config_options.append("-d")

	if env["platform"] == "windows":
		ssl_config_options.append("enable-capieng")

	return ssl_config_options




def get_platform_target(env):
	targets = {}
	platform = env["platform"]
	if platform == "linux":
		targets = {
			"x86_32": "linux-x86",
			"x86_64": "linux-x86_64",
			"arm64": "linux-aarch64",
			"arm32": "linux-armv4",
			"rv64": "linux64-riscv64",
		}
	elif platform == "android":
		targets = {
			"arm64": "android-arm64",
			"arm32": "android-arm",
			"x86_32": "android-x86",
			"x86_64": "android-x86_64",
		}
	elif platform == "macos":
		targets = {
			"x86_64": "darwin64-x86_64",
			"arm64": "darwin64-arm64",
		}
	elif platform == "ios":
		if env["ios_simulator"]:
			targets = {
				"x86_64": "iossimulator-xcrun",
				"arm64": "iossimulator-xcrun",
			}
		else:
			targets = {
				"arm64": "ios64-xcrun",
				"arm32": "ios-xcrun",
			}
	elif platform == "windows":
		if env.get("is_msvc", False):
			targets = {
				"x86_32": "VC-WIN32",
				"x86_64": "VC-WIN64A",
			}
		else:
			targets = {
				"x86_32": "mingw",
				"x86_64": "mingw64",
			}

	arch = env["arch"]
	target = targets.get(arch, "")
	if target == "":
		raise ValueError("Architecture '%s' not supported for platform: '%s'" % (arch, platform))
	return target






def update_openssl():
	git_cmd = ["git pull", "--recurse-submodules"]
	subprocess.run(git_cmd, shell=True, cwd="3party/openssl")

#./Configure --cross-compile-prefix=x86_64-w64-mingw32- mingw64


def compile_openssl(env):
	print()
#	make dclean
#	./Configure
#	make depend
#	make all

	return 0