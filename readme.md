# Joy-Con Side UDP

Command-line utility to forward Joy-Con side button sequences to a UDP listener.

## Dependencies

- Windows 11 64-bit.
- Make.
- MinGW-GCC.
- Bash.

## Building

Execute `make` to build the executable.

## Usage

Execute `./dist/joy-con-side-udp.exe 127.0.0.1 6772` to forward command
sequences entered by holding SL or SR on a Joy-Con pair connected via Bluetooth
and pressing other SL/SR buttons to port 6772 on localhost.

| Button   | Value |
| -------- | ----- |
| None     | 0     |
| Left SL  | 1     |
| Left SR  | 2     |
| Right SR | 3     |
| Right SL | 4     |

Pressing left SL then right SL while holding left SR would
generate the following bytes:

| Byte | Description                 |
| ---- | --------------------------- |
| 8    | Length of command.          |
| 0    |                             |
| 0    |                             |
| 0    |                             |
| 3    | Magic number.               |
| 0    |                             |
| 0    |                             |
| 0    |                             |
| 2    | Holding left SR.            |
| 1    | Pressed left SL.            |
| 4    | Pressed right SL.           |
| 0    | No further buttons pressed. |

Note that Steam may intercept these keys by default.  At the time of writing,
you can disable this interception in:

Steam > Settings > Controller > (layout) > Edit > Enable Rear Buttons > Off.
