# Operating Systems Capstone 2022

## Author

* Student ID：`B102011`
* GitHub account：`ssrtw`
* Name：`孫學任`
* Email：`pixar9899@gmail.com`

## compiler info

```
|  gcc   | version |
|--------|---------|
|binutils|  2.30   |
|gcc     |  8.1.0  |
|mpfr    |  4.0.1  |
|gmp     |  6.1.2  |
|mpc     |  1.1.0  |
|isl     |  0.18   |
|cloog   |  0.18.1 |
```

[ref](https://github.com/bztsrc/raspi3-tutorial/tree/master/00_crosscompiler#download-and-unpack-sources)

## How to build

* Compile kernel image(`kernel8.img`)

```bash
> cd kernel
> make
```

* Build bootloader image(`bootloader.img`)
```bash
> cd btlder
> make
```

* Run on qemu

```bash
> make run
> make debug
```

* Clean object file

```bash
> make clean
```

## Content

| File       | Description                  |
|------------|------------------------------|
| boot.s     | entrypoint, init CPU & Mem   |
| compiler.h | some compiler optimize macro |
| gpio.h     | define GPIO address          |
| main.c     | main function                |
| mbox.*     | mbox                         |
| reboot.*   | reboot                       |
| shell.*    | my simple shell              |
| stdarg.h   | define va_list               |
| stddef.h   | standard type define         |
| string.*   | some string funciton         |
| uart.*     | uart                         |