#! /usr/bin/env python

import os
import sys
from shutil import copyfile

def print_help():
	print "Usage: python plan.py <planner> <domain-route> <problem-route> <generator-route>"
	print ":: Example 1: python bin/plan.py she domains/AllenAlgebra/domain/domain.pddl domains/AllenAlgebra/problems/pfile10.pddl domains/AllenAlgebra/problems/generator"
	print ":: Example 2: python bin/plan.py tempo-2 domains/tempo-sat/Driverlog/domain/domain.pddl domains/tempo-sat/Driverlog/problems/p1.pddl" 

if __name__ == "__main__":
	if len(sys.argv) < 4:
		print_help()
		exit(1)
	
	baseFolder = os.path.dirname(os.path.realpath(sys.argv[0] + "/.."))
	
	inputPlanner = sys.argv[1]
	inputDomain = sys.argv[2]
	inputProblem = sys.argv[3]
	
	genTempoDomain = "tdom.pddl"
	genTempoProblem = "tins.pddl"
	
	genClassicDomain = "dom.pddl"
	genClassicProblem = "ins.pddl"
	
	## generate temporal domains or copy them to working directory
	if len(sys.argv) >= 5:
		inputGenerator = sys.argv[4]
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
	elif inputPlanner == "she-costs":
		compileCmd = "%s/bin/compileSHECosts %s %s > %s 2> %s" % (baseFolder, genTempoDomain, genTempoProblem, genClassicDomain, genClassicProblem)
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
	
	if inputPlanner == "she" or inputPlanner == "she-costs":
		planCmd = "python %s/fd_copy/fast-downward.py --build release64 --alias seq-sat-lama-2011 %s %s" % (baseFolder, genClassicDomain, genClassicProblem)
	elif inputPlanner.startswith("tempo"):
		planCmd = "python %s/fd_copy/fast-downward.py --build release64 --alias tp-lama %s %s" % (baseFolder, genClassicDomain, genClassicProblem)
	
	print "Planning: %s" % (planCmd)
	os.system(planCmd)

	## convert classical solutions into temporal solutions for she and she-costs
	if inputPlanner == "she" or inputPlanner == "she-costs":
		solFiles = [i for i in os.listdir(".") if i.startswith("sas_plan.")]
		solFiles.sort(reverse=True)
		if len(solFiles) == 0:
			print "Error: No solution to be converted into temporal has been found"
		else:
			_, numSol = solFiles[0].split(".")
			scheduleCmd = "%s/bin/planSchedule %s %s %s %s > tmp_sas_plan.%s" % (baseFolder, genTempoDomain, genClassicDomain, genTempoProblem, solFiles[0], numSol)
			print "Creating temporal plan: %s" % (scheduleCmd)
			os.system(scheduleCmd)


