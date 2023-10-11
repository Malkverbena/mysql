#!/usr/bin/env python3
# helpers.py

import os, subprocess, platform
from tools import boost
from tools import openssl


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

	if config["compile_boost"] == True:
		print("Compiling boost", config["compile_boost"])
		boost.compile_boost(env)

	if config["compile_openssl"] == True:
		print("Compiling openssl", config["compile_openssl"])
		openssl.compile_openssl(env)



def detect_gcc_version():
	system = platform.system()
	try:
		# Tenta obter a versão do GCC no Linux usando o comando "gcc --version"
		if system == 'Linux':
			output = subprocess.check_output(['gcc', '--version'], stderr=subprocess.STDOUT, text=True)
			print(output)
			return output

		# Tenta obter a versão do GCC no Windows usando o comando "gcc --version"
		elif system == 'Windows':
			output = subprocess.check_output(['gcc', '--version'], stderr=subprocess.STDOUT, text=True)
			print(output)
			return output

	except FileNotFoundError:
		print("GCC not found. Make sure GCC is installed and configured on your system.")



def detect_clang_version():
	system = platform.system()
	
	if system == 'Linux':
		try:
			# Executa o comando 'clang --version' no terminal
			result = subprocess.check_output(['clang', '--version'], stderr=subprocess.STDOUT, text=True)
			
			# Analisa a saída para extrair a versão do Clang
			lines = result.strip().split('\n')
			for line in lines:
				if line.startswith('clang version'):
					return line.split(' ')[2]
		except FileNotFoundError:
			print("Clang not found on Linux system.")

	elif system == 'Windows':
		try:
			# Executa o comando 'clang --version' no prompt de comando
			result = subprocess.check_output(['clang', '--version'], stderr=subprocess.STDOUT, text=True, shell=True)
			
			# Analisa a saída para extrair a versão do Clang
			lines = result.strip().split('\n')
			for line in lines:
				if line.startswith('clang version'):
					return line.split(' ')[2]
		except FileNotFoundError:
			return "Clang not found on Windows system."
	else:
		return "Operating system not supported."

