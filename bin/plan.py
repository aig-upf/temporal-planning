#! /usr/bin/env python

import os
import sys
from shutil import copyfile
import argparse

defaultGenerator = None
defaultTime = 3600
defaultMemory = 4096
defaultPlanPrefix = "sas_plan"
defaultIteratedSolution = True
defaultValidate = True

def getArguments():
    argParser = argparse.ArgumentParser()
    argParser.add_argument("planner", help="name of the planner (she, seq, tempo-1, tempo-2, stp-1, stp-2, ...)")
    argParser.add_argument("domain", help="input PDDL domain")
    argParser.add_argument("problem", help="input PDDL problem")
    argParser.add_argument("--generator", "-g", default=defaultGenerator, help="generator")
    argParser.add_argument("--time", "-t", default=defaultTime, help="maximum number of seconds during which the planner will run (default: %s seconds)" % defaultTime)
    argParser.add_argument("--memory", "-m", default=defaultMemory, help="maximum amount of memory in MiB to be used by the planner (default: %s MiB)" % defaultMemory)
    argParser.add_argument("--iterated", dest="iterated", action="store_true", help="look for more solutions after finding the first one")
    argParser.add_argument("--no-iterated", dest="iterated", action="store_false", help="stop after finding the first solution")
    argParser.add_argument("--plan-file", "-p", dest="planfile", default=defaultPlanPrefix, help="prefix of the resulting plan files")
    argParser.add_argument("--validate", dest="validate", action="store_true", help="validate the resulting plan")
    argParser.add_argument("--no-validate", dest="validate", action="store_false", help="do not validate the resulting plan")
    argParser.set_defaults(iterated=defaultIteratedSolution)
    argParser.set_defaults(validate=defaultValidate)
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

def getLastPlanFileName(planFilePrefix):
    solFiles = [i for i in os.listdir(".") if i.startswith("tmp_" + planFilePrefix)]
    solFiles.sort(reverse=True)
    if len(solFiles) == 0:
        return None
    else:
        return solFiles[0]

def runProblemGenerator(inputDomain, inputProblem, inputGenerator, genTempoDomain, genTempoProblem):
    if inputGenerator is not None:
        inputGeneratorCall = ""
        if inputGenerator.startswith("/"):
            inputGeneratorCall = inputGenerator
        else:
            inputGeneratorCall = "./" + inputGenerator
        generatorCmd = "%s %s %s > %s 2> %s" % (inputGeneratorCall, inputDomain, inputProblem, genTempoDomain, genTempoProblem)
        print "Executing generator: %s" % (generatorCmd)
        os.system(generatorCmd)
    else:
        copyfile(inputDomain, genTempoDomain)
        copyfile(inputProblem, genTempoProblem)

def runCompilation(baseFolder, inputPlanner, genTempoDomain, genTempoProblem, genClassicDomain, genClassicProblem):
    compileCmd = None

    if inputPlanner == "she":
        compileCmd = "%s/bin/compileSHE %s %s > %s 2> %s" % (baseFolder, genTempoDomain, genTempoProblem, genClassicDomain, genClassicProblem)
    elif inputPlanner == "seq":
        compileCmd = "%s/bin/compileSequential %s %s > %s 2> %s" % (baseFolder, genTempoDomain, genTempoProblem, genClassicDomain, genClassicProblem)
    elif inputPlanner.startswith("tempo-"):
        _, bound = inputPlanner.split("-")
        compileCmd = "%s/bin/compileTempo %s %s %s > %s 2> %s" % (baseFolder, genTempoDomain, genTempoProblem, bound, genClassicDomain, genClassicProblem)
    elif inputPlanner.startswith("stp-"):
        _, bound = inputPlanner.split("-")
        compileCmd = "%s/bin/compileTempoParallel %s %s %s > %s 2> %s" % (baseFolder, genTempoDomain, genTempoProblem, bound, genClassicDomain, genClassicProblem)

    print "Compiling problem: %s" % (compileCmd)
    os.system(compileCmd)

def runFastDownward(baseFolder, inputPlanner, genClassicDomain, genClassicProblem, timeLimit, memoryLimit, iteratedSolution, planFilePrefix):
    fdBuild = getFastDownwardBuildName(baseFolder)

    if fdBuild is None:
        print "Error: No Fast Downward compilation found. Compile Fast Downward before running this script."
        exit(-1)

    planCmd = None

    if inputPlanner == "she" or inputPlanner == "seq":
        ## clean current temporal solutions (in case of she, they are not deleted by fast-downward)
        planFiles = [i for i in os.listdir(".") if i.startswith(planFilePrefix) or i.startswith("tmp_" + planFilePrefix)]
        for planFile in planFiles:
            os.remove(planFile)
        aliasName = None
        if iteratedSolution:
            aliasName = "seq-sat-lama-2011"
        else:
            aliasName = "seq-sat-lama-2011-ni"
        planCmd = "python %s/fd_copy/fast-downward.py --build %s --alias %s --overall-time-limit %ss --overall-memory-limit %s --plan-file %s %s %s" % (baseFolder, fdBuild, aliasName, timeLimit, memoryLimit, planFilePrefix, genClassicDomain, genClassicProblem)
    elif inputPlanner.startswith("tempo") or inputPlanner.startswith("stp"):
        planCmd = "python %s/fd_copy/fast-downward.py --build %s --alias tp-lama --overall-time-limit %ss --overall-memory-limit %s --plan-file %s %s %s" % (baseFolder, fdBuild, timeLimit, memoryLimit, planFilePrefix, genClassicDomain, genClassicProblem)

    print "Compiling temporal problem: %s" % (planCmd)
    os.system(planCmd)

def transformClassicalSolutions(baseFolder, inputPlanner, genTempoDomain, genClassicDomain, genTempoProblem, iteratedSolution, planFilePrefix):
    if inputPlanner == "she" or inputPlanner == "seq":
        solFiles = [i for i in os.listdir(".") if i.startswith(planFilePrefix)]
        solFiles.sort(reverse=True)
        if len(solFiles) == 0:
            print "Error: No solution to be converted into temporal has been found"
        else:
            scheduleCmd = None
            if iteratedSolution:
                for solution in solFiles:
                    _, numSol = solution.split(".")
                    scheduleCmd = "%s/bin/planSchedule %s %s %s %s > tmp_%s.%s" % (baseFolder, genTempoDomain, genClassicDomain, genTempoProblem, solution, planFilePrefix, numSol)
                    print "Creating temporal plan: %s" % (scheduleCmd)
                    os.system(scheduleCmd)
            else:
                scheduleCmd = "%s/bin/planSchedule %s %s %s %s > tmp_%s" % (baseFolder, genTempoDomain, genClassicDomain, genTempoProblem, solFiles[0], planFilePrefix)
                os.system(scheduleCmd)

def runValidator(baseFolder, inputPlanner, genTempoDomain, genTempoProblem, planFilePrefix):
    if os.path.isfile("plan.validation"):
        os.remove("plan.validation")
    validatorBinary = os.path.join(baseFolder, "VAL/bin/Validate")
    if existsValidator(validatorBinary):
        lastPlan = getLastPlanFileName(planFilePrefix)
        if lastPlan is None:
            print "Error: No plan to validate was found"
        else:
            timePrecision = 0.001
            if inputPlanner == "she" or inputPlanner == "seq":
                timePrecision = 0.0001
            elif inputPlanner.startswith("tempo") or inputPlanner.startswith("stp"):
                timePrecision = 0.001
            valCmd = "%s -v -t %s %s %s %s > plan.validation" % (validatorBinary, timePrecision, genTempoDomain, genTempoProblem, lastPlan)
            print "Validating plan: %s" % valCmd
            os.system(valCmd)
    else:
        print "Error: Could not find the validator (current path: %s)" % validatorBinary

def runPlanner(baseFolder, inputPlanner, inputDomain, inputProblem,
               timeLimit=defaultTime, memoryLimit=defaultMemory,
               inputGenerator=defaultGenerator, iteratedSolution=defaultIteratedSolution,
               planFilePrefix=defaultPlanPrefix, validateSolution=defaultValidate):
    ## check if planner is accepted
    if not (inputPlanner == "she" or inputPlanner == "seq" or inputPlanner.startswith("tempo-") or inputPlanner.startswith("stp-")):
        print "Error: The specified planner '%s' is not recognized" % inputPlanner
        exit(-1)

    ## name of input domain and problems to she, tempo
    genTempoDomain = "tdom.pddl"
    genTempoProblem = "tins.pddl"

    ## name of files produced by she, tempo
    genClassicDomain = "dom.pddl"
    genClassicProblem = "ins.pddl"

    ## generate temporal domains or copy them to working directory
    runProblemGenerator(inputDomain, inputProblem, inputGenerator, genTempoDomain, genTempoProblem)

    ## convert temporal domains and problems into classical domains and problems
    runCompilation(baseFolder, inputPlanner, genTempoDomain, genTempoProblem, genClassicDomain, genClassicProblem)

    ## run fast downward
    runFastDownward(baseFolder, inputPlanner, genClassicDomain, genClassicProblem, timeLimit, memoryLimit, iteratedSolution, planFilePrefix)

    ## convert classical solutions into temporal solutions for she
    transformClassicalSolutions(baseFolder, inputPlanner, genTempoDomain, genClassicDomain, genTempoProblem, iteratedSolution, planFilePrefix)

    ## plan validation
    if validateSolution:
        runValidator(baseFolder, inputPlanner, genTempoDomain, genTempoProblem, planFilePrefix)

if __name__ == "__main__":
    args = getArguments()

    baseFolder = os.path.dirname(os.path.realpath(os.path.join(sys.argv[0], "..")))

    ## read input
    inputPlanner = args.planner
    inputDomain = args.domain
    inputProblem = args.problem
    timeLimit = args.time
    memoryLimit = args.memory
    inputGenerator = args.generator
    iteratedSolution = args.iterated
    planFilePrefix = args.planfile
    validateSolution = args.validate

    runPlanner(baseFolder, inputPlanner, inputDomain, inputProblem, timeLimit, memoryLimit, inputGenerator, iteratedSolution, planFilePrefix, validateSolution)
