#! /usr/bin/env python

import os
import sys
import argparse
import time
from shutil import copyfile
import plan

availableTime = 5 * 60  # num seconds available to find a solution

def getArguments():
    argParser = argparse.ArgumentParser()
    argParser.add_argument("domain", help="input PDDL domain")
    argParser.add_argument("problem", help="input PDDL problem")
    return argParser.parse_args()

def getRemainingTime(startTime):
    return availableTime - (time.time() - startTime)

def copyPlanFile(planFilename):
    copyfile(planFilename, "tmp_sas_plan")

if __name__ == "__main__":
    args = getArguments()

    startTime = time.time()

    baseFolder = os.path.dirname(os.path.realpath(os.path.join(sys.argv[0], "..")))

    inputDomain = args.domain
    inputProblem = args.problem

    plan.runPlanner(baseFolder, "seq", inputDomain, inputProblem, timeLimit=120, planFilePrefix="seq_sas_plan", validateSolution=True)
    lastSeqPlan = plan.getLastPlanFileName("seq_sas_plan")
    if lastSeqPlan is not None:
        copyPlanFile(lastSeqPlan)
        print ":: SEQUENTIAL SOLUTION FOUND ::"
        exit(0)

    plan.runPlanner(baseFolder, "she", inputDomain, inputProblem, timeLimit=120, planFilePrefix="she_sas_plan", validateSolution=True)
    lastShePlan = plan.getLastPlanFileName("she_sas_plan")
    if lastShePlan is not None:
        copyPlanFile(lastSeqPlan)
        print ":: SINGLE HARD ENVELOPE SOLUTION FOUND ::"
        exit(0)

    tempo_algs = ["tempo-2", "tempo-3", "tempo-4"]
    timePerAlg = int(getRemainingTime(startTime) / len(tempo_algs))

    for ta in tempo_algs:
        plan.runPlanner(baseFolder, ta, inputDomain, inputProblem, timeLimit=timePerAlg, planFilePrefix="%s_sas_plan" % ta, validateSolution=True)
        lastTempoPlan = plan.getLastPlanFileName("%s_sas_plan" % ta)
        if lastTempoPlan is not None:
            copyPlanFile(lastTempoPlan)
            print ":: %s SOLUTION FOUND ::" % ta.upper()
            exit(0)

    stp_algs = ["stp-2", "stp-3", "stp-4"]
    timePerAlg = int(getRemainingTime(startTime) / len(tempo_algs))

    for ta in stp_algs:
        plan.runPlanner(baseFolder, ta, inputDomain, inputProblem, timeLimit=timePerAlg, planFilePrefix="%s_sas_plan" % ta, validateSolution=True)
        lastTempoPlan = plan.getLastPlanFileName("%s_sas_plan" % ta)
        if lastTempoPlan is not None:
            copyPlanFile(lastTempoPlan)
            print ":: %s SOLUTION FOUND ::" % ta.upper()
            exit(0)
