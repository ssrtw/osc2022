# Operating Systems Capstone 2022

## Author

* Student ID：`B102011`
* GitHub account：`ssrtw`
* Name：`孫學任`
* Email：`pixar9899@gmail.com`

## How to build

* Compile kernel image(`kernel8.img`)

```bash
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