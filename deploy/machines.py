"""
Module defining how we configure the fabric environment for target machines.
Environment is loaded from YAML dictionaries machines.yml and machines_user.yml
"""
from fabric.api import *
import os
import subprocess
import posixpath
import yaml
from templates import *
from functools import *

#Root of local HemeLB checkout.
env.localroot=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
env.no_ssh=False
env.no_hg=False
#Load and invoke the default non-machine specific config JSON dictionaries.
config=yaml.load(open(os.path.join(env.localroot,'deploy','machines.yml')))
env.update(config['default'])
user_config=yaml.load(open(os.path.join(env.localroot,'deploy','machines_user.yml')))
env.update(user_config['default'])
env.verbose=False
env.needs_tarballs=False
env.cmake_options={}
env.pather=posixpath
env.remote=None
env.machine_name=None

@task
def machine(name):
	"""
	Load the machine-specific configurations.
	Completes additional paths and interpolates them, via complete_environment.
	Usage, e.g. fab machine:hector build
	"""
	if "import" in config[name]:
		# Config for this machine is based on another
		env.update(config[config[name]["import"]])
		if config[name]["import"] in user_config:
			env.update(user_config[config[name]["import"]])
	env.update(config[name])
	if name in user_config:
		env.update(user_config[name]) 
	env.machine_name=name
	complete_environment()

#Metaprogram the machine wrappers
for machine_name in set(config.keys())-set(['default']):
	globals()[machine_name]=task(alias=machine_name)(partial(machine,machine_name))

def complete_environment():
	"""Add paths to the environment based on information in the JSON configs.
	Templates are filled in from the dictionary to allow $foo interpolation in the JSON file.
	Environment vars created can be used in job-script templates:
	results_path: Path to store results
	remote_path: Root of area for checkout and build on remote
	config_path: Path to store config files
	repository_path: Path of remote mercurial checkout
	tools_path: Path of remote python 'Tools' folder
	tools_build_path: Path of disttools python 'build' folder for python tools
	regression_test_path: Path on remote to diffTest
	build_path: Path on remote to HemeLB cmake build area.
	install_path: Path on remote to HemeLB cmake install area.
	scripts_path: Path where job-queue-submission scripts generated by Fabric are sent.
	cmake_flags: Flags to pass to cmake
	run_prefix: Command string to invoke before any job is run.
	build_prefix: Command string to invoke before builds/installs are attempted
	build_number: Tip revision number of mercurial repository.
	build_cache: CMakeCache.txt file on remote, used to capture build flags.
	"""
	env.hosts=['%s@%s'%(env.username,env.remote)]
	env.home_path=template(env.home_path_template)
	env.runtime_path=template(env.runtime_path_template)
	env.work_path=template(env.work_path_template)
	env.remote_path=template(env.remote_path_template)
	env.install_path=template(env.install_path_template)
		
	env.results_path=env.pather.join(env.work_path,"results")
	env.config_path=env.pather.join(env.work_path,"config_files")
	env.profiles_path=env.pather.join(env.work_path,"profiles")
	env.scripts_path=env.pather.join(env.work_path,"scripts")
	env.build_path=env.pather.join(env.remote_path,'build')
	env.code_build_path=env.pather.join(env.remote_path,'code_build')
	env.repository_path=env.pather.join(env.remote_path,env.repository)
	
	env.temp_path=template(env.temp_path_template)
	
	env.tools_path=env.pather.join(env.repository_path,"Tools")
	env.regression_test_source_path=env.pather.join(env.repository_path,"RegressionTests","diffTest")
	env.regression_test_path=template(env.regression_test_path_template)
	env.tools_build_path=env.pather.join(env.install_path,env.python_build,'site-packages')
	
	env.cmake_total_options=env.cmake_default_options.copy()
	env.cmake_total_options.update(env.cmake_options)
	env.cmake_flags=' '.join(["-D%s=%s"%option for option in env.cmake_total_options.iteritems()])
	
	module_commands=["module %s"%module for module in env.modules]
	env.build_prefix=" && ".join(module_commands+env.build_prefix_commands) or 'echo Building...'
	
	env.run_prefix_commands.append("export PYTHONPATH=$$PYTHONPATH:$tools_build_path")
	env.run_prefix_commands.append(template("export TMP=$temp_path"))
	env.run_prefix_commands.append(template("export TMPDIR=$temp_path"))
	env.run_prefix=" && ".join(module_commands+map(template,env.run_prefix_commands)) or 'echo Running...'
	env.template_key = env.template_key or env.machine_name
	#env.build_number=subprocess.check_output(['hg','id','-i','-rtip','%s/%s'%(env.hg,env.repository)]).strip()
	# check_output is 2.7 python and later only. Revert to oldfashioned popen.
	cmd=os.popen(template("hg id -i -rtip $hg/$repository"))
	env.build_number=cmd.read().strip()
	cmd.close()
	#env.build_number=run("hg id -i -r tip")
	env.build_cache=env.pather.join(env.build_path,'CMakeCache.txt')
	env.code_build_cache=env.pather.join(env.code_build_path,"CMakeCache.txt")

complete_environment()
	