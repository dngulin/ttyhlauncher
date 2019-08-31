#ifndef PROFILESOPRESULTS_H
#define PROFILESOPRESULTS_H

namespace Ttyh {
namespace Profiles {
enum class CreateResult { Success, AlreadyExists, InvalidName, IOError };
enum class RenameResult {
    Success,
    OldNameDoesNotExist,
    NewNameAlreadyExists,
    InvalidName,
    IOError
};
}
}

#endif // PROFILESOPRESULTS_H
