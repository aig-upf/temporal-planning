(define (problem pfile0)
	(:domain testdomain)
	(:objects
		var1 - variable
	)
	(:init
		(norepeat var1)
	)
	(:goal
		(and
			(target1 var1)
			(target2 var1)
			(target3 var1)
		)
	)
	(:metric minimize (total-time))
)
