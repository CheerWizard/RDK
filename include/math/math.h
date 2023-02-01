
namespace rdk {

    template<typename T>
    T clamp(const T& value, const T& min, const T& max) {
        if (value < min)
            return min;
        else if (value >= min && value <= max)
            return value;
        else
            return max;
    }

}