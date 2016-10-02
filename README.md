[![Project Status: Active - The project has reached a stable, usable state and is being actively developed.](http://www.repostatus.org/badges/latest/active.svg)](http://www.repostatus.org/#active)
[![Build Status](https://travis-ci.org/scrm/scrm.svg?branch=master)](https://travis-ci.org/scrm/scrm)
[![Coverage Status](https://coveralls.io/repos/github/scrm/scrm/badge.svg?branch=master)](https://coveralls.io/github/scrm/scrm?branch=master)

scrm
====

_scrm_ simulates the evolution of genetic sequences. It takes a neutral evolutionary model as input, 
and generates random sequences that evolved under the model. As coalescent simulator, it traces
the ancestry of the sampled sequences backwards in time and is therefore extremely efficient. Compared to 
other coalescent simulators, it can simulate chromosome-scale sequences without a measureable reduction of
genetic linkage between different sites.


## Installation
### Stable Release (recommended) 
You can download the latest stable release packaged for a variety of different
platform from [_scrm_'s homepage][1]. 
Instructions on building the binary from the source packages are available in the [wiki][3].

### Development Version From GitHub

You can also install `scrm` directly from the git repository. Here, you need to install `autoconf` first:  

On Debian/Ubuntu based systems:
```bash
apt-get install build-essential autoconf autoconf-archive libcppunit-dev
```

On Mac OS:
```bash
port install automake autoconf autoconf-archive cppunit 
```

Afterwards you can build the binary using 
```bash
./bootstrap
make
```

## Usage
We designed scrm to be compatible to the famous program `ms` from Richard R. Hudson. 
You can use it as a drop in replacement for `ms` if you avoid the options `-c` and `-s`. 
Details are available [in the wiki][2]. 


## Troubleshooting
If you encounter problems while using _scrm_, please 
[file a bug report](https://github.com/scrm/scrm/wiki/Reporting-Bugs) or mail to
`develop (at) paulstaab.de`.


## Citation
_scrm_ is described in the manuscript

> Paul R. Staab, Sha Zhu, Dirk Metzler and Gerton Lunter.
> **scrm: efficiently simulating long sequences using the approximated coalescent
> with recombination**. 
> Bioinformatics (2015) 31 (10): 1680-1682.
> [doi:10.1093/bioinformatics/btu861](http://bioinformatics.oxfordjournals.org/content/31/10/1680).

## Licence
You can freely use all code in this project under the conditions of the GNU
GPL Version 3 or later.

[1]: https://scrm.github.io
[2]: https://github.com/paulstaab/scrm/wiki/Command-Line-Options
[3]: https://github.com/scrm/scrm/wiki/Installation
