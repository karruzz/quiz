# About
Tool for learning foreign languages
![alt text](https://raw.githubusercontent.com/karruzz/quiz/master/samples/screenshot.png)

# Dependencies
libncurses5-dev

# Build
```sh
$ mkdir build
$ cd build
```

base mode:
```sh
$ cmake ..
```

debug mode:
```sh
$ cmake -Ddebug=Y ..
```

with gtest:
```sh
$ cmake -DGTEST_DIR="your_path_to/Gtest/googletest" ..
```

# Start
```sh
$ cat ../samples/test.qz
# simple quiz for observing main capabilities

@ task_1
Translate phrase to Russian
-
%rus
> {{task_1}}
  hello world
< привет мир
> {{task_1}}
  -How are you?
  -Fine
< -Как дела?
  -Хорошо

# can use mixed mode for "d" topic
%deu
> yes, exactly
< ja, genau
> no, unfortunately
< nein, leider
```

help:
```sh
$ ./quiz -h
-h	show this help
-p	play the solution
-q	play the question
-m	mixed mode, question and solution may be swapped
-i	invert questions and solutions, discard mixed mode (-m)
-s	show statistic
-r	include to quiz problems, which were with errors last time only
-l	input language auto-detect
-e	accept answer by enter key
-t	use topics
-c	case unsensitive
```

start test.qz, words from "deu" topic only, mixed mode, accept by "enter" key:
```sh
$ ./quiz ../samples/test.qz -t deu -me
```

show statistic:
```sh
$ ./quiz ../samples/test.qz -s
? Translate phrase to Russian
-
hello world
> total errors: 0; last errors: 0;

? Translate phrase to Russian
-
-How are you?
-Fine
> total errors: 0; last errors: 0;

? yes, exactly
> total errors: 4; last errors: 3;

? no, unfortunately
> total errors: 4; last errors: 3;

```

# Use sublime syntax file for convenient editing qz files
```sh
$ cp sublime/quiz.sublime-syntax ~/.config/sublime-text-3/Packages/User/
```

![alt text](https://raw.githubusercontent.com/karruzz/quiz/master/sublime/qz_edit.png)