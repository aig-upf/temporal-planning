(define (problem aa-combo-1)
 (:domain allen-algebra)
 (:objects
	i1 i2 i3 - interval
 )
 (:init
	(not-started i1)
	(not-ended i1)
	(not-started i2)
	(not-ended i2)
	(not-started i3)
	(not-ended i3)
	(= (length i1) 5)
	(= (length i2) 11)
	(= (length i3) 5)
 )
 (:goal
	(and
		(starts i1 i2)
		(finishes i3 i2)
	)
 )
)
