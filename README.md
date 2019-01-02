** dependence **
libncurses5-dev

** compile **
> $ mkdir build
> $ cd build

base mode:
> $ cmake ..

debug mode:
> $ cmake -Ddebug=Y ..

# with gtest:
> $ cmake -DGTEST_DIR="/home/kar/Work/Gtest/googletest" ..

** start **
> $ cat ../samples/test.qz
> # simple quiz for observing main capabilities
>
> @ task_1
> Translate phrase to Russian
> -
> %rus
> > {{task_1}}
>   hello world
> < привет мир
> > {{task_1}}
>   -How are you?
>   -Fine
> < -Как дела?
>   -Хорошо
>
> # can use mixed mode for "d" topic
> %deu
> > yes, exactly
> < ja, genau
> > no, unfortunately
> < nein, leider

help:
> $ ./quiz -h

start test.qz, words from "deu" topic only, mixed mode, accept by "enter" key:
> $ ./quiz ../samples/test.qz -t deu -me

show statistic:
> $ ./quiz ../samples/test.qz -s
> ? Translate phrase to Russian
> -
> hello world
> > total errors: 0; last errors: 0;
>
> ? Translate phrase to Russian
> -
> -How are you?
> -Fine
> > total errors: 0; last errors: 0;
>
> ? yes, exactly
> > total errors: 4; last errors: 3;
>
> ? no, unfortunately
> > total errors: 4; last errors: 3;
>


** Use sublime syntax file for convinient editing qz files **

> $ cp sublime/quiz.sublime-syntax ~/.config/sublime-text-3/Packages/User/

![alt text](https://github.com/karruzz/quiz/sublime/qz_edit.png)