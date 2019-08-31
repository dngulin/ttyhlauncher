#ifndef DEFER_H
#define DEFER_H

#include <functional>

namespace Ttyh {
namespace Utils {

struct Defer {
    std::function<void()> action;

    explicit Defer(std::function<void()> doLater) : action(std::move(doLater)) {}
    ~Defer() { action(); }
};

}
}

#endif // DEFER_H
