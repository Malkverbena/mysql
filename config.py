# config.py

import version


def can_build(env, platform):
	return True


def configure(env):
	pass


def get_doc_path():
	return "doc_classes" + str(version.major)


def get_doc_classes():
	return [ 
	    "MySQL", 
	]


def get_icons_path():
	return "icons"
