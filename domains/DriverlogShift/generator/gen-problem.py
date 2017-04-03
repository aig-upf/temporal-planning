#! /usr/bin/env python
import sys,time

#**************************************#
# MAIN
#**************************************#
try:
   ndrivers = int(sys.argv[1])
   ntrucks = int(sys.argv[2])
   npackages = int(sys.argv[3])
   nlocs = int(sys.argv[4])   
except:
   print "Usage:"
   print sys.argv[0] + " <ndrivers>" + " <ntrucks>"+ " <npackages>"+ " <nlocs>"
   sys.exit(-1)


str_problem=""
str_problem=str_problem + "(define (problem DLOG-"+str(ndrivers)+"-"+str(ntrucks)+"-"+str(npackages)+"-"+str(nlocs)+"-"+time.strftime("%d%m%Y%H%M%S")+")\n"
str_problem=str_problem + "  (:domain driverlogshift)\n"
str_problem=str_problem + "  (:objects "

for j in range(0,ndrivers):
   str_problem=str_problem + "d"+str(j)+" "
str_problem=str_problem + "- driver "

for j in range(0,ntrucks):
   str_problem=str_problem + "t"+str(j)+" "
str_problem=str_problem + "- truck "

for j in range(0,npackages):
   str_problem=str_problem + "p"+str(j)+" "
str_problem=str_problem + "- obj "

for j in range(0,nlocs):
   str_problem=str_problem + "l"+str(j)+" "
for j in range(0,nlocs-1):
   str_problem=str_problem + "p"+str(j)+"-"+str(j+1)+" "

str_problem=str_problem + "- location)\n"

str_problem=str_problem + "  (:init \n"
for j in range(0,ndrivers):
   str_problem=str_problem + "\t(at d" + str(j) + " l" + str(nlocs-1) + ") \n"
   str_problem=str_problem + "\t(rested d"+str(j)+") \n"

for j in range(0,ntrucks):
   str_problem=str_problem + "\t(at t" + str(j) + " l0) \n"   
   str_problem=str_problem + "\t(empty t"+str(j)+") \n"

for j in range(0,npackages):
   str_problem=str_problem + "\t(at p" + str(j) + " l0) \n"

for i in range(0,nlocs):
   for j in range(0,nlocs):
      if not i==j:
         str_problem=str_problem + "\t(link l"+str(i)+" l"+str(j)+")\n"

for j in range(0,nlocs-1):
   str_problem=str_problem + "\t(path l"+str(j)+" p"+str(j)+"-"+str(j+1)+")\n"
   str_problem=str_problem + "\t(path p"+str(j)+"-"+str(j+1)+" l"+str(j)+")\n"
   str_problem=str_problem + "\t(path l"+str(j+1)+" p"+str(j)+"-"+str(j+1)+")\n"
   str_problem=str_problem + "\t(path p"+str(j)+"-"+str(j+1)+" l"+str(j+1)+")\n"   
str_problem=str_problem + ")\n"   

str_problem=str_problem + "  (:goal (and "

for j in range(0,ndrivers):
   str_problem=str_problem + "(at d" + str(j) + " l"+str(nlocs-1)+") "
   
for j in range(0,ntrucks):
   str_problem=str_problem + "(at t" + str(j) + " l"+str(nlocs-1)+") "

for j in range(0,npackages):
   str_problem=str_problem + "(at p" + str(j) + " l" + str(nlocs-1) + ") "

str_problem=str_problem + ")))\n"

f_problem=open("new-problem.pddl","w")
f_problem.write(str_problem)
f_problem.close()

sys.exit(0)
