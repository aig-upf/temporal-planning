# Temporal Planning

This documentation aims to explain how the experiments with the planners introduced by [[Jiménez, Jonsson and Palacios, 2015]](#ref-tmp-planning-icaps15) can be run.

1. [Installation](#installation)
	1. [Universal PDDL Parser](#universal-pddl-parser)
	1. [Planner Compilation](#planner-compilation)
1. [Usage](#usage)
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

#### Fast Downward Compilation

#### Compilation


## <a name="usage"></a>Usage

## <a name="credits"></a>Credits

The planner is a modified version of the [Fast Downward](http://www.fast-downward.org) distribution.

## <a name="references"></a>References

* <a name="ref-tmp-planning-icaps15">Jiménez, S., Jonsson, A., and Palacios, H. (2015).</a> [_Temporal Planning With Required Concurrency Using Classical Planning_](http://www.dtic.upf.edu/~jonsson/icaps15.pdf). Proceedings of the 25th International Conference on Automated Planning and Scheduling.