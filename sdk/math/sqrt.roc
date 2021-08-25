package math

fun Sqrt(x f64) -> f64 {
    ret ccall<f64>("Sqrt", x)
}
