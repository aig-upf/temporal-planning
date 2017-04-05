# Temporal Planning

This documentation aims to explain how the experiments with the planners introduced by [[Jiménez, Jonsson and Palacios, 2015]](#ref-tmp-planning-icaps15) can be run.

1. [Installation](#installation)
	1. [Universal PDDL Parser](#universal-pddl-parser)
	1. [Planner Compilation](#planner-compilation)
		1. [Fast Downward Compilation](#fd-compilation)
		1. [TPSHE and TP Compilation](#tpshe-tempo-compilation)
	1. [Domain Generator Compilation](#domain-generator-compilation)
1. [Usage](#usage)
	1. [Automatic Usage](#automatic-usage)
	1. [Manual Usage](#manual-usage)
		1. [Generator of Domains and Problems](#use-generator-dom-prob)
		1. [Running TPSHE](#use-tpshe)
		1. [Running TP](#use-tp)
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

The temporal solutions obtained by Fast Downward will be written to files whose name will begin with `tmp_sas_plan.` followed by a number indicating the solution number to the problem (e.g. `tmp_sas_plan.2` would be the second solution that solves the problem).

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

#### <a name="use-generator-dom-prob"></a>Generator of Domains and Problems

As explained before, a generator is used for the AllenAlgebra domain so as to obtain the temporal domains and problems. The generator is found in the `domains/AllenAlgebra/problems` folder.

The command for obtaining temporal domains and problems is the following:

```
./generator <input-domain>  <input-problem> > <output-domain> 2> <output-problem>
```

Assuming that we are in the folder `temporal-planning` (the root), an example would be:

```
./domains/AllenAlgebra/problems/generator domains/AllenAlgebra/domain/domain.pddl domains/AllenAlgebra/problems/pfile10.pddl > tdom.pddl 2> tins.pddl
```

#### <a name="use-tpshe"></a>Running TPSHE

To use `TPSHE`, you have to run the binary `compileSHE` placed in the `bin` folder. The command follows this structure:

```
./compileSHE <domain> <problem> > <output-domain> 2> <output-problem>
```

The following command is an example of how it is used for the Driverlog domain given that we are in the `temporal-planning` (the root) folder:

```
./bin/compileSHE domains/tempo-sat/Driverlog/domain/domain.pddl domains/tempo-sat/Driverlog/problems/p10.pddl > dom.pddl 2> ins.pddl
```

Once the domain and the problem have been converted, we can use Fast Downward using the LAMA-2011 setting. The command is the following (use the build you used to compile Fast Downward):

```
python fd_copy/fast-downward.py --build release64 --alias seq-sat-lama-2011 dom.pddl ins.pddl
```

Since LAMA-2011 is used for `TPSHE`, a classical plan will be obtained instead of a temporal plan. The name of such plans begins with `sas_plan`. To convert a classical plan into a temporal plan, you can use the `planSchedule` tool of the `bin` folder as follows:

```
./planSchedule <temporal-domain> <classical-domain> <temporal-problem> <classical-plan> > <temporal-plan>
```

#### <a name="use-tp"></a>Running TP

To use `TP`, you have to run the binary `compileTempo` placed in the `bin` folder. The command follows this structure:

```
./compileTempo <domain> <problem> <bound> > <output-domain> 2> <output-problem>
```

The following command is an example of how it is used for the Driverlog domain using bound 2 given that we are in the `temporal-planning` (the root) folder:

```
./bin/compileTempo domains/tempo-sat/Driverlog/domain/domain.pddl domains/tempo-sat/Driverlog/problems/p10.pddl 2 > dom.pddl 2> ins.pddl
```

Once the domain and the problem have been converted, we can use Fast Downward using the TP-LAMA setting. The command is the following (use the build you used to compile Fast Downward):

```
python fd_copy/fast-downward.py --build release64 --alias tp-lama dom.pddl ins.pddl
```

The output of Fast Downward will consist of temporal plans (the name of the files starts with `tmp_sas_plan.`) unlike with `TPSHE`.

## <a name="credits"></a>Credits

The planner is a modified version of the [Fast Downward](http://www.fast-downward.org) distribution to support temporal planning.

## <a name="references"></a>References

* <a name="ref-tmp-planning-icaps15">Jiménez, S., Jonsson, A., and Palacios, H. (2015).</a> [_Temporal Planning With Required Concurrency Using Classical Planning_](http://www.dtic.upf.edu/~jonsson/icaps15.pdf). Proceedings of the 25th International Conference on Automated Planning and Scheduling.