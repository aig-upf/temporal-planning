#! /usr/bin/env python

import os
import sys
from shutil import copyfile
import argparse

def getArguments():
	argParser = argparse.ArgumentParser()
	argParser.add_argument("planner", help="name of the planner (she, tempo-1, tempo-2, ...)")
	argParser.add_argument("domain", help="input PDDL domain")
	argParser.add_argument("problem", help="input PDDL problem")
	argParser.add_argument("--generator", "-g", default=None, help="generator")
	argParser.add_argument("--time", "-t", default=3600, help="maximum number of seconds during which the planner will run (default: 3600 seconds)")
	argParser.add_argument("--memory", "-m", default=4096, help="maximum amount of memory in MiB to be used by the planner (default: 4096 MiB)")
	argParser.add_argument("--iterated", dest="iterated", action="store_true", help="look for more solutions after finding the first one")
	argParser.add_argument("--no-iterated", dest="iterated", action="store_false", help="stop after finding the first solution")
	argParser.set_defaults(iterated=True)
	return argParser.parse_args()

def getFastDownwardBuildName(baseFolder):
	downwardBuilds = baseFolder + "/fd_copy/builds/"
	fdBuilds = ["release32", "release64", "debug32", "debug64", "minimal"]
	for build in fdBuilds:
		if os.path.isfile(downwardBuilds + build + "/bin/downward"):
			return build
	return None

def existsValidator(validatorBinary):
	return os.path.isfile(validatorBinary)

def getLastPlanFileName():
	solFiles = [i for i in os.listdir(".") if i.startswith("tmp_sas_plan")]
	solFiles.sort(reverse=True)
	if len(solFiles) == 0:
		return None
	else:
		return solFiles[0]

if __name__ == "__main__":
	args = getArguments()

	baseFolder = os.path.dirname(os.path.realpath(sys.argv[0] + "/.."))
	fdBuild = getFastDownwardBuildName(baseFolder)
	validatorBinary = baseFolder + "/../VAL/validate"

	if fdBuild is None:
		print "Error: No Fast Downward compilation found. Compile Fast Downward before running this script"
		exit(-1)

	## read input
	inputPlanner = args.planner
	inputDomain = args.domain
	inputProblem = args.problem
	timeLimit = args.time
	memoryLimit = args.memory
	inputGenerator = args.generator
	iteratedSolution = args.iterated

	## check if planner is accepted
	if not (inputPlanner == "she" or inputPlanner.startswith("tempo-")):
		print "Error: The specified planner '%s' is not recognized" % inputPlanner
		exit(-1)

	## name of input domain and problems to she, tempo
	genTempoDomain = "tdom.pddl"
	genTempoProblem = "tins.pddl"

	## name of files produced by she, tempo
	genClassicDomain = "dom.pddl"
	genClassicProblem = "ins.pddl"

	## generate temporal domains or copy them to working directory
	if inputGenerator is not None:
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

	print "Compiling problem: %s" % (compileCmd)
	os.system(compileCmd)

	## run fast downward
	planCmd = None

	if inputPlanner == "she":
		## clean current temporal solutions (in case of she, they are not deleted by fast-downward)
		planFiles = [i for i in os.listdir(".") if i.startswith("sas_plan") or i.startswith("tmp_sas_plan")]
		for planFile in planFiles:
			os.remove(planFile)
		aliasName = None
		if iteratedSolution:
			aliasName = "seq-sat-lama-2011"
		else:
			aliasName = "seq-sat-lama-2011-ni"
		planCmd = "python %s/fd_copy/fast-downward.py --build %s --alias %s --overall-time-limit %ss --overall-memory-limit %s %s %s" % (baseFolder, fdBuild, aliasName, timeLimit, memoryLimit, genClassicDomain, genClassicProblem)
	elif inputPlanner.startswith("tempo"):
		planCmd = "python %s/fd_copy/fast-downward.py --build %s --alias tp-lama --overall-time-limit %ss --overall-memory-limit %s %s %s" % (baseFolder, fdBuild, timeLimit, memoryLimit, genClassicDomain, genClassicProblem)

	print "Compiling temporal problem: %s" % (planCmd)
	os.system(planCmd)

	## convert classical solutions into temporal solutions for she
	if inputPlanner == "she":
		solFiles = [i for i in os.listdir(".") if i.startswith("sas_plan")]
		solFiles.sort(reverse=True)
		if len(solFiles) == 0:
			print "Error: No solution to be converted into temporal has been found"
		else:
			scheduleCmd = None
			if iteratedSolution:
				_, numSol = solFiles[0].split(".")
				scheduleCmd = "%s/bin/planSchedule %s %s %s %s > tmp_sas_plan.%s" % (baseFolder, genTempoDomain, genClassicDomain, genTempoProblem, solFiles[0], numSol)
			else:
				scheduleCmd = "%s/bin/planSchedule %s %s %s %s > tmp_sas_plan" % (baseFolder, genTempoDomain, genClassicDomain, genTempoProblem, solFiles[0])
			print "Creating temporal plan: %s" % (scheduleCmd)
			os.system(scheduleCmd)

	## plan validation
	if os.path.isfile("plan.validation"):
		os.remove("plan.validation")
	if existsValidator(validatorBinary):
		lastPlan = getLastPlanFileName()
		if lastPlan is None:
			print "Error: No plan to validate was found"
		else:
			timePrecision = 0.001
			if inputPlanner == "she":
				timePrecision = 0.0001
			elif inputPlanner.startswith("tempo"):
				timePrecision = 0.001
			valCmd = "%s -v -t %s %s %s %s > plan.validation" % (validatorBinary, timePrecision, genTempoDomain, genTempoProblem, lastPlan)
			print "Validating plan: %s" % valCmd
			os.system(valCmd);
	else:
		print "Error: Could not find the validator (current path: %s)" % validatorBinary
