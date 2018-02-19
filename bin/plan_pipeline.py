#! /usr/bin/env python

import os
import sys
import argparse
import time
from shutil import copyfile
from glob import glob
import plan

defaultTime = 30 * 60  # num seconds available to find a solution
defaultMemory = 6000
defaultValidate = False

def getArguments():
    argParser = argparse.ArgumentParser()
    argParser.add_argument("domain", help="input PDDL domain")
    argParser.add_argument("problem", help="input PDDL problem")
    argParser.add_argument("--plan-file", "-p", dest="planfile", default="tmp_sas_plan", help="name of the output temporal plan")
    argParser.add_argument("--time", "-t", default=defaultTime, type=int, help="maximum number of seconds available to find a solution")
    argParser.add_argument("--memory", "-m", default=defaultMemory, help="maximum amount of memory available to solve the problems")
    argParser.add_argument("--generator", "-g", default=None, help="generator")
    argParser.add_argument("--no-iterated", dest="iterated", action="store_false", help="stop after finding the first solution (it only works for sequential and tpshe planners)")
    argParser.add_argument("--validate", dest="validate", action="store_true", help="validate the resulting plan")
    argParser.add_argument("--no-validate", dest="validate", action="store_false", help="do not validate the resulting plan")
    argParser.add_argument("--use-full-time", dest="usefulltime", action="store_true", help="each planner tries to use the whole amount of time")
    argParser.set_defaults(iterated=True)
    argParser.set_defaults(validate=defaultValidate)
    argParser.set_defaults(usefulltime=False)
    return argParser.parse_args()

def getRemainingTime(startTime, availableTime):
    return availableTime - (time.time() - startTime)

def getElapsedTime(startTime):
    return time.time() - startTime

def copyPlanFile(planFilename, outputPlanFilename):
    copyfile(planFilename, outputPlanFilename)

def runPlanner(baseFolder, args, startTime, planner, timeLimit):
    planPrefix = "%s_sas_plan" % planner

    print "starting technique: %s %s" % (planner, getElapsedTime(startTime))
    plan.runPlanner(baseFolder, planner, args.domain, args.problem, timeLimit=timeLimit, memoryLimit=args.memory, planFilePrefix=planPrefix, iteratedSolution=args.iterated, validateSolution=args.validate, inputGenerator=args.generator)
    print "technique finished: %s %s" % (planner, getElapsedTime(startTime))

    lastPlan = plan.getLastPlanFileName(planPrefix)
    if lastPlan is not None:
        copyPlanFile(lastPlan, args.planfile)
        for fl in glob("*" + planPrefix + "*"):
            os.remove(fl)
        print "most useful technique: %s" % planner
        exit(0)

def planSequential(baseFolder, args, startTime):
    if args.usefulltime:
        timeLimit = args.time
    else:
        timeLimit = int(args.time * 0.25)
    runPlanner(baseFolder, args, startTime, "seq", timeLimit)

def planSHE(baseFolder, args, startTime):
    if args.usefulltime:
        timeLimit = int(getRemainingTime(startTime, args.time))
    else:
        timeLimit = int(args.time * 0.25)
    runPlanner(baseFolder, args, startTime, "she", timeLimit)

def planTempo(baseFolder, args, startTime):
    tempo_algs = ["tempo-2", "tempo-3", "tempo-4"]
    timePerAlg = int(getRemainingTime(startTime, args.time) / len(tempo_algs))
    for ta in tempo_algs:
        runPlanner(baseFolder, args, startTime, ta, timePerAlg)

def planSTP(baseFolder, args, startTime):
    stp_algs = ["stp-2", "stp-3", "stp-4"]
    timePerAlg = int(getRemainingTime(startTime, args.time) / len(stp_algs))
    for ta in stp_algs:
        runPlanner(baseFolder, args, startTime, ta, timePerAlg)

if __name__ == "__main__":
    args = getArguments()
    startTime = time.time()
    baseFolder = os.path.dirname(os.path.realpath(os.path.join(sys.argv[0], "..")))

    planSequential(baseFolder, args, startTime)
    planSHE(baseFolder, args, startTime)
    planTempo(baseFolder, args, startTime)
    planSTP(baseFolder, args, startTime)
