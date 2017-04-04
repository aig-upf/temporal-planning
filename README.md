# Temporal Planning

This documentation aims to explain how the experiments with the planners introduced by [[Jiménez, Jonsson and Palacios, 2015]](#ref-tmp-planning-icaps15) can be run.

1. [Installation](#installation)
	1. [Universal PDDL Parser](#universal-pddl-parser)
	1. [Planner Compilation](#planner-compilation)
		1. [Fast Downward Compilation](#fd-compilation)
		1. [TP-SHE and TEMPO Compilation](#tpshe-tempo-compilation)
		1. [Domain Generator Compilation](#domain-generator-compilation)
1. [Usage](#usage)
	1. [Automatic Usage](#automatic-usage)
	1. [Manual Usage](#manual-usage)
1. [Credits](#credits)
1. [References](#references)

## <a name="installation"></a>Installation

### <a name="universal-pddl-parser"></a>Universal PDDL Parser

The input domains and problems will be parsed using the [Universal PDDL Parser](https://github.com/aig-upf/universal-pddl-parser). You can clone it by executing the following command:

```
git clone https://github.com/aig-upf/universal-pddl-parser.git
```

Afterwards, you should compile it using `scons`:

```
cd universal-pddl-parser
scons
```

### <a name="planner-compilation"></a>Planner Compilation

Firstly, you have to either clone or download this repository. It is important to have the folder next to the `universal-pddl-parser` folder so that it can be properly referenced. To clone it, you can use the following command:

```
git clone https://github.com/aig-upf/temporal-planning.git
```

#### <a name="fd-compilation"></a>Fast Downward Compilation

The Fast Downward version in this repository contains modifications to support temporal planning. However, the steps for compiling it are the same (see section "Compiling the planner" [here](http://www.fast-downward.org/ObtainingAndRunningFastDownward).

For example, in case you wanted to compile the 64-bit version you should run the following commands:

```
cd temporal-planning
python fd_copy/build.py release64
```

#### <a name="tpshe-tempo-compilation"></a>TPSHE and TP Compilation

The planners `TPSHE` and `TP` can be compiled by running the `scons` command in the root directory:

```
cd temporal-planning
scons
```

After running these commands, executable files will be created in the `temporal-planning/bin` directory.

### <a name="domain-generator-compilation"></a>Domain Generator Compilation

The Allen Algebra domain requires a generator to create the problems that will be used as the planner's input. You can compile this generator by running the following commands:

```
cd temporal-planning/domains/AllenAlgebra
scons
```

After executing such commands, a file called `generator` will be created inside `AllenAlgebra/problems`.

## <a name="usage"></a>Usage

### <a name="automatic-usage"></a>Automatic Usage

A Python script called `plan.py` inside the `bin` folder encapsulates all the required calls in order to get a plan given a planner (`TPSHE` or `TP`) and a temporal planning problem. You can run it as follows:

```
python plan.py <planner> <domain-path> <problem-path> <generator-path>
```

where:

* `<planner>`: Name of the algorithm you want to use.

	* If you want to use `TPSHE`, you must write `she`.

	* If you want to use `TP`, you must write `tempo-i` where `i` is the bound you want to use. For example, for bound 2 you should use `tempo-2`, while for bound 3 you should use `tempo-3`.

* `<domain-path>`: Path to the input domain.

* `<problem-path>`: Path to the input problem.

* `<generator-path>`: Path to the executable generator for transforming the input domain and problem. It is just needed for the Allen Algebra domain.

#### Example 1

```
cd temporal-planning
python bin/plan.py she domains/AllenAlgebra/domain/domain.pddl domains/AllenAlgebra/problems/pfile10.pddl domains/AllenAlgebra/problems/generator
```

#### Example 2

```
cd temporal-planning
python bin/plan.py tempo-2 domains/tempo-sat/Driverlog/domain/domain.pddl domains/tempo-sat/Driverlog/problems/p1.pddl
```

### <a name="manual-usage"></a>Manual Usage

## <a name="credits"></a>Credits

The planner is a modified version of the [Fast Downward](http://www.fast-downward.org) distribution to support temporal planning.

## <a name="references"></a>References

* <a name="ref-tmp-planning-icaps15">Jiménez, S., Jonsson, A., and Palacios, H. (2015).</a> [_Temporal Planning With Required Concurrency Using Classical Planning_](http://www.dtic.upf.edu/~jonsson/icaps15.pdf). Proceedings of the 25th International Conference on Automated Planning and Scheduling.