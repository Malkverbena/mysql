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



def get_config():
	conf_file = "config.cfg"
	conf = {} 

	with open("config.cfg", 'r') as f:
		lines = f.readlines()

		for line in lines:
			line = line.strip()
			if not line or line.startswith("#"):
				continue

			key, value = line.split("=")
			key = key.strip()
			value = value.strip()
			if key in ["update_boost", "update_openssl", "compile_boost", "compile_openssl"]:
				value = value.lower() == 'true'
#			else if key in []:
#				value = int(value)

			conf[key] = value

	return conf



def apply_config(env):

	config = get_config()

#	print("MySQL DEPENDENCIES CONFIG: ", config)

	if config["update_boost"] == True:
		print("Updating boost", config["update_boost"])
		boost.update_boost()

	if config["update_openssl"] == True:
		print("Updating openssl", config["update_openssl"])
		openssl.update_openssl()

	if config["compile_openssl"] == True:
		print("Compiling openssl", config["compile_openssl"])
		openssl.compile_openssl(env)

	if config["compile_boost"] == True:
		print("Compiling boost", config["compile_boost"])
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

