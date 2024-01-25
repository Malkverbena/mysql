#!/usr/bin/env python3
# boost.py


# https://www.boost.org/build/tutorial.html


import os, subprocess
from tools import helpers


def compile_boost(env):

	jobs = "-j" + str(env.GetOption("num_jobs"))
	boost_path = f"{os.getcwd()}/3party/boost"
	
	boost_prefix = get_boost_install_path(env)		# --exec-prefix=
	boost_stage_dir = f"{boost_prefix}/stage"		# --exec-prefix=
	boost_lib_dir = f"{boost_stage_dir}/lib"		# --libdir=
	boost_include_dir = f"{boost_prefix}/include"	# --includedir=
	build_dir = f"{boost_prefix}/build"				# --build-dir=


	is_win_host = (helpers.get_host() == "windows")
	bootstrap = "bootstrap.bat" if is_win_host else "bootstrap.sh"
	b2 = "b2.exe" if is_win_host else "b2"

	bootstrap_command = [
		"bash", bootstrap,
		f"--prefix={boost_prefix}",
		f"--libdir={boost_lib_dir}",
		f"--includedir={boost_include_dir}"
	]
	compiller = helpers.get_compiller(env)
	if compiller == "clang":
		bootstrap_command.append("--with-toolset=clang")

	if not os.path.exists(boost_prefix):
		os.makedirs(boost_prefix)


	try:
		subprocess.check_call(bootstrap_command, cwd=boost_path, env={"PATH": f"{boost_path}:{os.environ['PATH']}"})
	except subprocess.CalledProcessError as e:
		print(f"Erro ao configurar o Boost: {e}")
		exit(1)



	is_cross_compilation = is_cross_compile(env)
	if is_cross_compilation:
		project_comp = get_project_set(env)
		with open('project-config.jam', 'a') as arquivo:
			arquivo.write(project_comp + "\n")


	bits = "64" if env["arch"] in ["x86_64", "arm64", "rv64", "ppc64"] else "32"
	cmd_b2 = [b2]
	cmd_b2 += [
		jobs,
		"-a",
		"link=static",
		"threading=multi",
		"runtime-link=static",
		"variant=release",
		f"--build-dir={build_dir}",
		f"--exec-prefix={boost_prefix}",
		f"--stagedir={boost_stage_dir}",
		f"toolset={get_toolset(env)}",
		f"address-model={bits}",
		f"architecture={get_architecture(env)}",
		f"target-os={get_target_os(env)}",
		"headers"
	]

	print(cmd_b2)

	try:
		subprocess.check_call(cmd_b2, cwd=boost_path, env={"PATH": f"{boost_path}:{os.environ['PATH']}"})
		cmd_b2.pop()
		subprocess.check_call(cmd_b2, cwd=boost_path, env={"PATH": f"{boost_path}:{os.environ['PATH']}"})
	#	subprocess.check_call([b2, "install"], cwd=boost_path, env={"PATH": f"{boost_path}:{os.environ['PATH']}"})
	except subprocess.CalledProcessError as e:
		print(f"Erro ao Compilar o Boost: {e}")
	except Exception as e:
		print(f"Outro erro: {e}")
		exit(1)

	return 0



def get_project_set(env):
	target_platform = env["platform"]
	host_platform = helpers.get_host()
	target_bits = "64" if env["arch"] in ["x86_64", "arm64", "rv64", "ppc64"] else "32"
	compiller = helpers.get_compiller(env)

	if host_platform == "linuxbsd":
		if target_platform ==  "windows":
			if compiller == "gcc":
				if target_bits == "64":
					"using gcc : mingw : x86_64-w64-mingw32-g++ ;"
				else:
					"using gcc : mingw : i686-w64-mingw32-g++ ;"
			elif compiller == "clang":
				if target_bits == "64":
					"using clang : : x86_64-w64-mingw32-clang++ ;"
				else:
					"using clang : : i686-w64-mingw32-clang++ ;"

	if host_platform == "windows":
		#TODO:
		pass




def is_cross_compile(env):
	host_bits = helpers.get_host_bits()
	host_platform = helpers.get_host()
	target_platform = env["platform"]
	target_bits = "64" if env["arch"] in ["x86_64", "arm64", "rv64", "ppc64"] else "32"
	return (host_platform != target_platform or host_bits != target_bits)



def get_target_os(env):
	if env["platform"] == "windows":
		return "windows"
	elif env["platform"] == "linuxbsd":
		return "linux"
	else:
		return ""




def get_toolset(env):
	compiller = helpers.get_compiller(env)
	if compiller == "":
		return ""

	if env["platform"] == "windows":
		if compiller == "gcc":
			return "gcc-mingw"
		elif compiller == "clang":
			return "clang-mingw"
		else:
			return ""

	if env["platform"] == "linuxbsd":
		return compiller

	return ""



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



def get_boost_install_path(env):
	bin_path = f"{os.getcwd()}/3party/bin"
	_lib_path = [bin_path, env["platform"], env["arch"]]
	if env["use_llvm"]:
		_lib_path.append("llvm")
	_lib_path.append("boost")
	lib_path = "/".join(_lib_path)
	return lib_path

