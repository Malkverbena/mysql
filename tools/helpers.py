#!/usr/bin/env python3
# helpers.py

import platform, sys
from tools import boost, openssl



def get_host_bits():
	my_arch = platform.architecture()
	bits = "64" if  my_arch[0] == "64bit" else "32"
	return bits



# Allowed platforms: ["linuxbsd", "macos", "windows"]
def get_host():

	if sys.platform in ["win32", "msys", "cygwin"]:
		return "windows"

	elif sys.platform in ["macos", "osx", "darwin"]:
		return "macos"

	elif ( sys.platform in ["linux", "linux2"] 
		or sys.platform.startswith("linux")
		or sys.platform.startswith("dragonfly")
		or sys.platform.startswith("freebsd")
		or sys.platform.startswith("netbsd")
		or sys.platform.startswith("openbsd")):
		return "linuxbsd"
	
	return "unknown"


def apply_config(env):

	if env["recompile_sql"] in ["all", "openssl"]:
		openssl.compile_openssl(env)

	if env["recompile_sql"] in ["all", "boost"]:
		boost.compile_boost(env)



def get_compiller(env):

	host_platform = get_host()
	if env["platform"] == "linuxbsd":
		return "clang" if env["use_llvm"] else "gcc"
	if env["platform"] == "windows":
		if not env["use_mingw"]:
			return "msvc"
		else:
			if host_platform == "linuxbsd":
				return "clang" if env["use_llvm"] else "gcc"
			else:
				# Compilador rodando no MacOS
				pass

	return ""

