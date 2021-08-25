package main

fun adder (
a int, b
int) ->
int {
    ret
    a +
    b
}

fun
box() -> Bool {
    if adder(12, 25) == 37 {
        ret true
    }
    ret false
}
