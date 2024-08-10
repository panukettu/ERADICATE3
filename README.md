# ERADICATE2

```
usage: ./ERADICATE2 [OPTIONS]

  input (create3):
    -d3   --c3-deployer               Address of the create3 proxy deployer (set this!)
    -c3   --c3-proxy-hash             Inithash of temp proxy [default:"21c35dbe1b344a2488cf3321d6ce542f8e9f305544ff09e4993a62319a497c1f"]

  input (create2):
    -d,   --deployer                  Create2 deployer address
    -I,   --init-code                 Init code
    -i,   --init-code-file            Read init code from this file

  config:
    -ms   --min-score                 Min score to save into output file [default: 6 / 2 (leading-match and matching)]
    -f    --file                      Filename to output results into [default: "Mode-timestamp.txt"]

  modes:
    -b    --benchmark                 Run a benchmark with no scoring.
    -z    --zero-bytes                Score on zero bytes in hash.
    -Z    --zeros                     Score on zeros anywhere in hash.
    -L    --letters                   Score on letters anywhere in hash.
    -N    --numbers                   Score on numbers anywhere in hash.
    -mr   --mirror                    Score on mirroring from center.
    -ld   --leading-doubles           Score on hashes leading with hexadecimal pairs.
    -al   --all-leading               Score on any successive leading character.
    -alt  --all-leading-trailing      Score on any successive leading and trailing

  modes with arguments:
    -a    --all <min-score>           Hashes above min-score with any successive hex characters
    -l    --leading <nibble>          Score on hashes leading with given hex character.
    -t    --trailing <nibble>         Score on hashes trailing with given hex character.
    -x    --matching <hexstr>         Score on hashes matching given hex string.
    -lx   --leading-match <hexstr>    Score on hashes leading with given hex string.
    -lt   --leading-trailing <2nibble>Score on hashes with successive leading (1st nibble) and trailing (2nd nibble).

  range modes:
    -lr   --leading-range             Scores on hashes leading with characters within given range.
    -r    --range                     Scores on hashes having characters within given range anywhere.
  range arguments:
    -m,   --min <0-15>                Set range minimum (inclusive), 0 is '0' 15 is 'f'.
    -M,   --max <0-15>                Set range maximum (inclusive), 0 is '0' 15 is 'f'.

  device control:
    -s,   --skip <index>              Skip device given by index.
    -n,   --no-cache                  Don't load cached pre-compiled version of kernel.

  tweaking:
    -w,   --work <size>               Set OpenCL local work size. [default: 64]
    -W,   --work-max <size>           Set OpenCL maximum work size. [default: -i * -I]
    -S,   --size <size>               Set number of salts tried per loop.[default: 16777216]

  examples:
    ./ERADICATE2 -d3 0x00000000000000000000000000000000deadbeef -l 0 -ms 6    (0x000000...)
    ./ERADICATE2 -d3 0x00000000000000000000000000000000deadbeef -lt a1 -ms 3  (0xaaa...111)
    ./ERADICATE2 -d3 0x00000000000000000000000000000000deadbeef -alt -ms 4    (0x***...***)
    ./ERADICATE2 -d3 0x00000000000000000000000000000000deadbeef -al -ms 4     (0x******...)
    ./ERADICATE2 -d3 0x00000000000000000000000000000000deadbeef -lx 123123    (0x123123...)

  about:
    ERADICATE2 is a vanity address generator for CREATE2 addresses that
	utilizes computing power from GPUs using OpenCL.

    Author: Johan Gustafsson <johan@johgu.se>
    Beer donations: 0x000dead000ae1c8e8ac27103e4ff65f42a4e9203
```
