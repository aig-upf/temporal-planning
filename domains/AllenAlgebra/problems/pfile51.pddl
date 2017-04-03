(define (problem aa-during-3)
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
	(= (length i2) 10)
	(= (length i3) 15)
 )
 (:goal
	(and
		(during i1 i2)
		(during i2 i3)
	)
 )
)
