#!/usr/bin/env python3
# boost.py

import os, subprocess, sys


boost_cmd = [
	"-a",
	"link=static",
	"threading=multi",
	"runtime-link=static",
]


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



def update_boost():
	git_cmd = ["git pull", "--recurse-submodules"]
	subprocess.run(git_cmd, shell=True, cwd="3party/boost")


def get_variant(env):
	if env["target"] == "editor":
		return "debug"
	elif env["target"] == "template_debug":
		return "profile"
	elif env["target"] == "template_release":
		return "release"



def get_lto(env):
	if env["lto"] == "none":
		return "fat"
	elif env["lto"] == "thin":
		return "thin"		
	elif env["lto"] == "full":
		return "full"


# Allowed boot values: x86, ia64, sparc, power, mips, mips1, mips2, mips3, mips4, mips32, mips32r2, mips64, parisc, arm, s390x, loongarch.
# Supports ["x86", "arm", "power", riscv"] for now
def get_architecture(env):
	godot_target = env["arch"]
	if "x86" in godot_target:
		return "x86"
	elif "arm" in godot_target:
		return "arm"
	elif "ppc" in godot_target:
		return "power"
	elif "rv" in godot_target:
		return "riscv"
	elif "wasm" in godot_target:
		return "????"


def execute(env, commands, tool):
	print(commands)
	host = get_host()
	print("HOST:" + host + " Base system: " + sys.platform + " Target platform: " + env["platform"])
	print("###############################")
	comp = f"--with-toolset={tool}"

	caminho_executavel = f"{os.getcwd()}/3party/boost"
	os.chdir(caminho_executavel)

	is_win_host = sys.platform in ["win32", "msys", "cygwin"]

	print("SYS PLATFORM: ", sys.platform)
	bootstrap = "bootstrap.bat" if is_win_host else "bootstrap.sh"
	b2 = "b2.exe" if is_win_host else "./b2"

	configure_command = [
		"bash", os.path.join(caminho_executavel, bootstrap),
		f"--with-toolset={tool}",
	]


	try:
		subprocess.check_call(configure_command)
	except subprocess.CalledProcessError as e:
		print(f"Erro ao configurar o Boost: {e}")
		exit(1)


	cmd_b2 = [b2] + commands
	print(cmd_b2)

	os.chdir(caminho_executavel)
	subprocess.run(cmd_b2)

	cmd_b2.append("headers")
	subprocess.run(cmd_b2)



# Allowed platforms:  ["linuxbsd", "macos", "windows", "ios", "android", "web"]
def	get_target_platform(plat):
	print("PLATFORM: ", plat)
	if plat == "linuxbsd":
		return "linux"
	elif plat == "macos" or plat == "ios":
		return "darwin"
	elif plat == "windows":
		return "windows"
	elif plat == "android":
		return "android"




def compile_boost(env):

	jobs = "30" 

	target_bits = "64" if "64" in env["arch"] else "32"

	target_architecture = get_architecture(env) 

	target_variant = get_variant(env)

#	lto = env["use_lld"]

	lto_mode = get_lto(env)

	target_platform = get_target_platform(env["platform"])

	architecture = f"architecture={target_architecture}"
	address_model = f"address-model={target_bits}"
	variant = f"variant={target_variant}"
	target_os = f"target-os={target_platform}" 
	user_config = "--user-config=../godot-config.jam"


	host = get_host()

	tool = ""

	commands = [f"-j{jobs}"]


	if host == "linuxbsd":

		if env["platform"] == "linuxbsd":
			tool = "clang" if env["use_llvm"] else "gcc"

		elif env["platform"] == "windows":
			tool = "gcc"

		elif env["platform"] == "macos":
			pass

	if host == "windows":
		if env["platform"] == "linuxbsd":
			tool = "clang" if env["use_llvm"] else "gcc"

		elif env["platform"] == "windows":
			tool = "msvc"

		elif env["platform"] == "macos":
			pass


	print(target_os)

	commands.extend([
		target_os,
		architecture,
		address_model,
		variant,
		user_config,
		f"toolset={tool}",
	])
	
	commands +=	boost_cmd


	execute(env, commands, tool)



