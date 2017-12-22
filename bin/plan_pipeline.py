#! /usr/bin/env python

import os
import sys
import argparse
import time
from shutil import copyfile
import plan

availableTime = 30 * 60  # num seconds available to find a solution
validateSolution = True
memoryLimit = 6000

def getArguments():
    argParser = argparse.ArgumentParser()
    argParser.add_argument("domain", help="input PDDL domain")
    argParser.add_argument("problem", help="input PDDL problem")
    return argParser.parse_args()

def getRemainingTime(startTime):
    return availableTime - (time.time() - startTime)

def getElapsedTime(startTime):
    return time.time() - startTime

def copyPlanFile(planFilename):
    copyfile(planFilename, "tmp_sas_plan")

if __name__ == "__main__":
    args = getArguments()

    startTime = time.time()

    baseFolder = os.path.dirname(os.path.realpath(os.path.join(sys.argv[0], "..")))

    inputDomain = args.domain
    inputProblem = args.problem

    # SEQUENTIAL SOLUTION
    plan.runPlanner(baseFolder, "seq", inputDomain, inputProblem, timeLimit=8 * 60, memoryLimit=memoryLimit, planFilePrefix="seq_sas_plan", validateSolution=validateSolution)
    lastSeqPlan = plan.getLastPlanFileName("seq_sas_plan")
    if lastSeqPlan is not None:
        copyPlanFile(lastSeqPlan)
        print ":: SEQUENTIAL SOLUTION FOUND ::"
        print ":: ELAPSED TIME - %s ::" % getElapsedTime(startTime)
        exit(0)

    # SINGLE HARD ENVELOPE (SHE) SOLUTION
    plan.runPlanner(baseFolder, "she", inputDomain, inputProblem, timeLimit=8 * 60, memoryLimit=memoryLimit, planFilePrefix="she_sas_plan", validateSolution=validateSolution)
    lastShePlan = plan.getLastPlanFileName("she_sas_plan")
    if lastShePlan is not None:
        copyPlanFile(lastShePlan)
        print ":: SINGLE HARD ENVELOPE SOLUTION FOUND ::"
        print ":: ELAPSED TIME - %s ::" % getElapsedTime(startTime)
        exit(0)

    # TEMPORAL PLAN WITHOUT SIMULTANEOUS EVENTS
    tempo_algs = ["tempo-2", "tempo-3", "tempo-4"]
    timePerAlg = int(getRemainingTime(startTime) / len(tempo_algs))

    for ta in tempo_algs:
        plan.runPlanner(baseFolder, ta, inputDomain, inputProblem, timeLimit=timePerAlg, memoryLimit=memoryLimit, planFilePrefix="%s_sas_plan" % ta, validateSolution=validateSolution)
        lastTempoPlan = plan.getLastPlanFileName("%s_sas_plan" % ta)
        if lastTempoPlan is not None:
            copyPlanFile(lastTempoPlan)
            print ":: %s SOLUTION FOUND ::" % ta.upper()
            print ":: ELAPSED TIME - %s ::" % getElapsedTime(startTime)
            exit(0)

    # TEMPORAL PLAN WITH SIMULTANEOUS EVENTS
    stp_algs = ["stp-2", "stp-3", "stp-4"]
    timePerAlg = int(getRemainingTime(startTime) / len(stp_algs))

    for ta in stp_algs:
        plan.runPlanner(baseFolder, ta, inputDomain, inputProblem, timeLimit=timePerAlg, memoryLimit=memoryLimit, planFilePrefix="%s_sas_plan" % ta, validateSolution=validateSolution)
        lastStpPlan = plan.getLastPlanFileName("%s_sas_plan" % ta)
        if lastStpPlan is not None:
            copyPlanFile(lastStpPlan)
            print ":: %s SOLUTION FOUND ::" % ta.upper()
            print ":: ELAPSED TIME - %s ::" % getElapsedTime(startTime)
            exit(0)
