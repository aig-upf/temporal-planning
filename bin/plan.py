#! /usr/bin/env python

import os
import sys
from shutil import copyfile

def get_fast_downward_build_name(baseFolder):
	downwardBuilds = baseFolder + "/fd_copy/builds/"
	fdBuilds = ["release32", "release64", "debug32", "debug64", "minimal"]
	
	for build in fdBuilds:
		if os.path.isfile(downwardBuilds + build + "/bin/downward"):
			return build
	
	return None

def print_help():
	print "Usage: python plan.py <planner> <domain-path> <problem-path> <time-limit> <generator-path>"
	print ":: Example 1: python bin/plan.py she domains/AllenAlgebra/domain/domain.pddl domains/AllenAlgebra/problems/pfile10.pddl 3600 domains/AllenAlgebra/problems/generator"
	print ":: Example 2: python bin/plan.py tempo-2 domains/tempo-sat/Driverlog/domain/domain.pddl domains/tempo-sat/Driverlog/problems/p1.pddl 3600" 

if __name__ == "__main__":
	if len(sys.argv) < 5:
		print_help()
		exit(1)
	
	baseFolder = os.path.dirname(os.path.realpath(sys.argv[0] + "/.."))
	fdBuild = get_fast_downward_build_name(baseFolder)
	
	if fdBuild is None:
		print "Error: No Fast Downward compilation found. Compile Fast Downward before running this script"
		exit(-1)
	
	inputPlanner = sys.argv[1]
	inputDomain = sys.argv[2]
	inputProblem = sys.argv[3]
	timeLimit = sys.argv[4]
	
	genTempoDomain = "tdom.pddl"
	genTempoProblem = "tins.pddl"
	
	genClassicDomain = "dom.pddl"
	genClassicProblem = "ins.pddl"
	
	## generate temporal domains or copy them to working directory
	if len(sys.argv) >= 6:
		inputGenerator = sys.argv[5]
		generatorCmd = "./%s %s %s > %s 2> %s" % (inputGenerator, inputDomain, inputProblem, genTempoDomain, genTempoProblem)
		print "Executing generator: %s" % (generatorCmd)
		os.system(generatorCmd)
	else:
		copyfile(inputDomain, genTempoDomain)
		copyfile(inputProblem, genTempoProblem)

	## convert temporal domains and problems into classical domains and problems
	compileCmd = None
	
	if inputPlanner == "she":
		compileCmd = "%s/bin/compileSHE %s %s > %s 2> %s" % (baseFolder, genTempoDomain, genTempoProblem, genClassicDomain, genClassicProblem)
	elif inputPlanner.startswith("tempo-"):
		_, bound = inputPlanner.split("-")
		compileCmd = "%s/bin/compileTempo %s %s %s > %s 2> %s" % (baseFolder, genTempoDomain, genTempoProblem, bound, genClassicDomain, genClassicProblem)

	if compileCmd is None:
		print "Error: The specified planner (%s) is not valid" % (inputPlanner)
		exit(-1)
	
	print "Compiling problem: %s" % (compileCmd)
	os.system(compileCmd)
	
	## run fast downward
	planCmd = None
	
	if inputPlanner == "she":
		planCmd = "python %s/fd_copy/fast-downward.py --build %s --alias seq-sat-lama-2011 --overall-time-limit %ss %s %s" % (baseFolder, fdBuild, timeLimit, genClassicDomain, genClassicProblem)
	elif inputPlanner.startswith("tempo"):
		planCmd = "python %s/fd_copy/fast-downward.py --build %s --alias tp-lama --overall-time-limit %ss %s %s" % (baseFolder, fdBuild, timeLimit, genClassicDomain, genClassicProblem)
	
	print "Planning: %s" % (planCmd)
	os.system(planCmd)

	## convert classical solutions into temporal solutions for she
	if inputPlanner == "she":
		solFiles = [i for i in os.listdir(".") if i.startswith("sas_plan.")]
		solFiles.sort(reverse=True)
		if len(solFiles) == 0:
			print "Error: No solution to be converted into temporal has been found"
		else:
			_, numSol = solFiles[0].split(".")
			scheduleCmd = "%s/bin/planSchedule %s %s %s %s > tmp_sas_plan.%s" % (baseFolder, genTempoDomain, genClassicDomain, genTempoProblem, solFiles[0], numSol)
			print "Creating temporal plan: %s" % (scheduleCmd)
			os.system(scheduleCmd)


