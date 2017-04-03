#! /usr/bin/env python
import os

#**************************************#
# MAIN
#**************************************#
problem_set=[[1,[1,1,1,2]],[1,[2,2,2,2]],[1,[3,2,2,2]],[1,[2,2,3,2]],[1,[3,3,3,3]],[1,[4,3,3,3]],[1,[3,3,4,3]],[1,[4,4,4,4]],[1,[5,4,4,4]],[1,[4,4,5,4]],[1,[5,5,5,5]],[1,[6,5,5,5]],[1,[5,5,6,5]],[1,[6,6,6,6]],[1,[7,6,6,6]],[1,[6,6,7,6]],[1,[7,7,7,7]],[1,[8,7,7,7]],[1,[7,7,8,7]],[1,[8,8,8,8]]]

counter=0
for config in problem_set:
   for i in range(0,config[0]):
      cmd = "rm new-problem.pddl; ./gen-problem.py "+ " ".join(map(str,config[1]))
      print "Executing... "+cmd
      os.system(cmd)

      cmd = "mv new-problem.pddl pfile"+str(counter)+".pddl"
      print "Executing... "+cmd
      os.system(cmd)
      counter=counter+1
exit(0)
