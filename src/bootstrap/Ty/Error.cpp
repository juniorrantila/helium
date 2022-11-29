#include "Error.h"
#include "Formatter.h"
#include "StringBuffer.h"

#ifndef __linux__
#    include <string.h>
#endif

namespace Ty {

namespace {

#if __linux__
enum class Errno : u32 {
    NoError = 0,
    Permission = 1,
    NoEntry = 2,
    Search = 3,
    Interrupted = 4,
    IO = 5,
    NoIO = 6,
    TooBig = 7,
    NoExec = 8,
    BadFile = 9,
    Child = 10,
    Again = 11,
    NoMemory = 12,
    Access = 13,
    Fault = 14,
    NotBlock = 15,
    Busy = 16,
    Exist = 17,
    CrossDeviceLink = 18,
    NoDev = 19,
    NotDir = 20,
    IsDir = 21,
    InvalidArgument = 22,
    NFile = 23,
    ManyFiles = 24,
    NoTTY = 25,
    TextBusy = 26,
    FileBig = 27,
    NoSpace = 28,
    SeekPipe = 29,
    ReadOnlyFS = 30,
    ManyLinks = 31,
    BrokenPipe = 32,
    MathOutsideDomain = 33,
    MathRange = 34,
    DeadLock = 35,
    NameTooLong = 36,
    NoLock = 37,
    InvalidSyscallNumber = 38,
    NotEmpty = 39,
    Loop = 40,
    NoMessage = 42,
    IdentifierRemoved = 43,
    ChannelRange = 44,
    L2NoSync = 45,
    L3Halted = 46,
    L3Reset = 47,
    LinkOutOfRange = 48,
    UNATCH = 49,
    NoCSI = 50,
    L2Halted = 51,
    BadExchange = 52,
    BadRequestDescriptor = 53,
    ExchangeFull = 54,
    NoAnode = 55,
    BadRequestCode = 56,
    BadSlot = 57,
    BadFont = 59,
    NoStream = 60,
    NoData = 61,
    TimeExpired = 62,
    NoStreamResources = 63,
    NoNetwork = 64,
    NoPackage = 65,
    Remote = 66,
    NoLink = 67,
    Advertise = 68,
    Srmount = 69,
    Comm = 70,
    Proto = 71,
    MultiHop = 72,
    DotDot = 73,
    BadMessage = 74,
    Overflow = 75,
    NotUnique = 76,
    BadFD = 77,
    RemoteChanged = 78,
    LibraryAccess = 79,
    BadLibrary = 80,
    LibrarySection = 81,
    MaxLibrary = 82,
    ExecLibrary = 83,
    IllegalSequence = 84,
    Restart = 85,
    StreamPipe = 86,
    Users = 87,
    NotSocket = 88,
    DestinationAddressRequired = 89,
    MessageSize = 90,
    Prototype = 91,
    NoProtocolOption = 92,
    ProtocolNotSupported = 93,
    SocketTypeNotSupported = 94,
    OperationNotSupported = 95,
    ProtocolFamilyNotSupported = 96,
    AddressFamilyNotSupported = 97,
    AddressInUse = 98,
    AddressNotAvailable = 99,
    NetworkDown = 100,
    NetworkUnrechable = 101,
    NetworkReset = 102,
    ConnectionAborted = 103,
    ConnectionReset = 104,
    NoBufferSpaceAvailable = 105,
    IsConnected = 106,
    NotConnected = 107,
    Shutdown = 108,
    TooManyRefs = 109,
    TimedOut = 110,
    ConnectionRefused = 111,
    HostDown = 112,
    HostUnrechable = 113,
    Already = 114,
    InProgress = 115,
    Stale = 116,
    NeedsCleaning = 117,
    NotXENIXTypeFile = 118,
    NoXENIXSemaphoresAvailable = 119,
    IsName = 120,
    RemoteIO = 121,
    QuotaExceeded = 122,
    NoMedium = 123,
    MediumType = 124,
    Cancelled = 125,
    NoKey = 126,
    KeyExpired = 127,
    KeyRevoked = 128,
    KeyRejected = 129,
    OwnerDead = 130,
    NotRecoverable = 131,
    RFKill = 132,
    HWPoison = 133,
};
consteval u32 operator&(Errno&& code) { return (u32)code; }

// clang-format off
constexpr const c_string error_codes[] {
    [&Errno::NoError] = "No error",
    [&Errno::Permission] = "Operation not permitted",
    [&Errno::NoEntry] = "No such file or directory",
    [&Errno::Search] = "No such process",
    [&Errno::Interrupted] = "Interrupted system call",
    [&Errno::IO] = "I/O error",
    [&Errno::NoIO] = "No such device or address",
    [&Errno::TooBig] = "Argument list too long",
    [&Errno::NoExec] = "Exec format error",
    [&Errno::BadFile] = "Bad file number",
    [&Errno::Child] = "No child processes",
    [&Errno::Again] = "Try again",
    [&Errno::NoMemory] = "Out of memory",
    [&Errno::Access] = "Permission denied",
    [&Errno::Fault] = "Bad address",
    [&Errno::NotBlock] = "Block device required",
    [&Errno::Busy] = "Device or resource busy",
    [&Errno::Exist] = "File exists",
    [&Errno::CrossDeviceLink] = "Cross-device link",
    [&Errno::NoDev] = "No such device",
    [&Errno::NotDir] = "Not a directory",
    [&Errno::IsDir] = "Is a directory",
    [&Errno::InvalidArgument] = "Invalid argument",
    [&Errno::NFile] = "File table overflow",
    [&Errno::ManyFiles] = "Too many open files",
    [&Errno::NoTTY] = "Not a TTY",
    [&Errno::TextBusy] = "Text file busy",
    [&Errno::FileBig] = "File too large",
    [&Errno::NoSpace] = "No space left on device",
    [&Errno::SeekPipe] = "Illegal seek",
    [&Errno::ReadOnlyFS] = "Read-only file system",
    [&Errno::ManyLinks] = "Too many links",
    [&Errno::BrokenPipe] = "Broken pipe",
    [&Errno::MathOutsideDomain] = "Math argument outside domain of function",
    [&Errno::MathRange] = "Math result not representable",
    [&Errno::DeadLock] = "Resource deadlock would occur",
    [&Errno::NameTooLong] = "File name too long",
    [&Errno::NoLock] = "No record locks available",
    [&Errno::InvalidSyscallNumber] = "Invalid system call number",
    [&Errno::NotEmpty] = "Directory not empty",
    [&Errno::Loop] = "Too many symbolic links encountered",
    [&Errno::NoMessage] = "No message of desired type",
    [&Errno::IdentifierRemoved] = "Identifier removed",
    [&Errno::ChannelRange] = "Channel number out of range",
    [&Errno::L2NoSync] = "Level 2 not synchronized",
    [&Errno::L3Halted] = "Level 3 halted",
    [&Errno::L3Reset] = "Level 3 reset",
    [&Errno::LinkOutOfRange] = "Link number out of range",
    [&Errno::UNATCH] = "Protocol driver not attached",
    [&Errno::NoCSI] = "No CSI structure available",
    [&Errno::L2Halted] = "Level 2 halted",
    [&Errno::BadExchange] = "Invalid exchange",
    [&Errno::BadRequestDescriptor] = "Invalid request descriptor",
    [&Errno::ExchangeFull] = "Exchange full",
    [&Errno::NoAnode] = "No anode",
    [&Errno::BadRequestCode] = "Invalid request code",
    [&Errno::BadSlot] = "Invalid slot",
    [&Errno::BadFont] = "Bad font file format",
    [&Errno::NoStream] = "Device not a stream",
    [&Errno::NoData] = "No data available",
    [&Errno::TimeExpired] = "Timer expired",
    [&Errno::NoStreamResources] = "Out of streams resources",
    [&Errno::NoNetwork] = "Machine is not on the network",
    [&Errno::NoPackage] = "Package not installed",
    [&Errno::Remote] = "Object is remote",
    [&Errno::NoLink] = "Link has been severed",
    [&Errno::Advertise] = "Advertise error",
    [&Errno::Srmount] = "Srmount error",
    [&Errno::Comm] = "Communication error on send",
    [&Errno::Proto] = "Protocol error",
    [&Errno::MultiHop] = "Multihop attempted",
    [&Errno::DotDot] = "RFS specific error",
    [&Errno::BadMessage] = "Not a data message",
    [&Errno::Overflow] = "Value too large for defined data type",
    [&Errno::NotUnique] = "Name not unique on network",
    [&Errno::BadFD] = "File descriptor in bad state",
    [&Errno::RemoteChanged] = "Remote address changed",
    [&Errno::LibraryAccess] = "Can not access a needed shared library",
    [&Errno::BadLibrary] = "Accessing a corrupted shared library",
    [&Errno::LibrarySection] = ".lib section in a.out corrupted",
    [&Errno::MaxLibrary] = "Attempting to link in too many shared libraries",
    [&Errno::ExecLibrary] = "Cannot exec a shared library directly",
    [&Errno::IllegalSequence] = "Illegal byte sequence",
    [&Errno::Restart] = "Interrupted system call should be restarted",
    [&Errno::StreamPipe] = "Streams pipe error",
    [&Errno::Users] = "Too many users",
    [&Errno::NotSocket] = "Socket operation on non-socket",
    [&Errno::DestinationAddressRequired] = "Destination address required",
    [&Errno::MessageSize] = "Message too long",
    [&Errno::Prototype] = "Protocol wrong type for socket",
    [&Errno::NoProtocolOption] = "Protocol not available",
    [&Errno::ProtocolNotSupported] = "Protocol not supported",
    [&Errno::SocketTypeNotSupported] = "Socket type not supported",
    [&Errno::OperationNotSupported] = "Operation not supported on transport endpoint",
    [&Errno::ProtocolFamilyNotSupported] = "Protocol family not supported",
    [&Errno::AddressFamilyNotSupported] = "Address family not supported by protocol",
    [&Errno::AddressInUse] = "Address already in use",
    [&Errno::AddressNotAvailable] = "Cannot assign requested address",
    [&Errno::NetworkDown] = "Network is down",
    [&Errno::NetworkUnrechable] = "Network is unreachable",
    [&Errno::NetworkReset] = "Network dropped connection because of reset",
    [&Errno::ConnectionAborted] = "Software caused connection abort",
    [&Errno::ConnectionReset] = "Connection reset by peer",
    [&Errno::NoBufferSpaceAvailable] = "No buffer space available",
    [&Errno::IsConnected] = "Transport endpoint is already connected",
    [&Errno::NotConnected] = "Transport endpoint is not connected",
    [&Errno::Shutdown] = "Cannot send after transport endpoint shutdown",
    [&Errno::TooManyRefs] = "Too many references: cannot splice",
    [&Errno::TimedOut] = "Connection timed out",
    [&Errno::ConnectionRefused] = "Connection refused",
    [&Errno::HostDown] = "Host is down",
    [&Errno::HostUnrechable] = "No route to host",
    [&Errno::Already] = "Operation already in progress",
    [&Errno::InProgress] = "Operation now in progress",
    [&Errno::Stale] = "Stale file handle",
    [&Errno::NeedsCleaning] = "Structure needs cleaning",
    [&Errno::NotXENIXTypeFile] = "Not a XENIX named type file",
    [&Errno::NoXENIXSemaphoresAvailable] = "No XENIX semaphores available",
    [&Errno::IsName] = "Is a named type file",
    [&Errno::RemoteIO] = "Remote I/O error",
    [&Errno::QuotaExceeded] = "Quota exceeded",
    [&Errno::NoMedium] = "No medium found",
    [&Errno::MediumType] = "Wrong medium type",
    [&Errno::Cancelled] = "Operation Canceled",
    [&Errno::NoKey] = "Required key not available",
    [&Errno::KeyExpired] = "Key has expired",
    [&Errno::KeyRevoked] = "Key has been revoked",
    [&Errno::KeyRejected] = "Key was rejected by service",
    [&Errno::OwnerDead] = "Owner died",
    [&Errno::NotRecoverable] = "State not recoverable",
    [&Errno::RFKill] = "Operation not possible due to RF-kill",
    [&Errno::HWPoison] = "Memory page has hardware error",
};
// clang-format on
constexpr const i32 error_codes_size
    = sizeof(error_codes) / sizeof(error_codes[0]);

#elif __APPLE__

enum class Errno : u32 {
    NoError = 0,
    PERM = 1,
    NOENT = 2,
    SRCH = 3,
    INTR = 4,
    IO = 5,
    NXIO = 6,
    TOOBIG = 7,
    NOEXEC = 8,
    BADF = 9,
    CHILD = 10,
    DEADLK = 11,
    NOMEM = 12,
    ACCES = 13,
    FAULT = 14,
    NOTBLK = 15,
    BUSY = 16,
    EXIST = 17,
    XDEV = 18,
    NODEV = 19,
    NOTDIR = 20,
    ISDIR = 21,
    INVAL = 22,
    NFILE = 23,
    MFILE = 24,
    NOTTY = 25,
    TXTBSY = 26,
    FBIG = 27,
    NOSPC = 28,
    SPIPE = 29,
    ROFS = 30,
    MLINK = 31,
    PIPE = 32,
    DOM = 33,
    RANGE = 34,
    AGAIN = 35,
    INPROGRESS = 36,
    ALREADY = 37,
    NOTSOCK = 38,
    DESTADDRREQ = 39,
    MSGSIZE = 40,
    PROTOTYPE = 41,
    NOPROTOOPT = 42,
    PROTONOSUPPORT = 43,
    SOCKTNOSUPPORT = 44,
    NOTSUP = 45,
    PFNOSUPPORT = 46,
    AFNOSUPPORT = 47,
    ADDRINUSE = 48,
    ADDRNOTAVAIL = 49,
    NETDOWN = 50,
    NETUNREACH = 51,
    NETRESET = 52,
    CONNABORTED = 53,
    CONNRESET = 54,
    NOBUFS = 55,
    ISCONN = 56,
    NOTCONN = 57,
    SHUTDOWN = 58,
    TOOMANYREFS = 59,
    TIMEDOUT = 60,
    CONNREFUSED = 61,
    LOOP = 62,
    NAMETOOLONG = 63,
    HOSTDOWN = 64,
    HOSTUNREACH = 65,
    NOTEMPTY = 66,
    PROCLIM = 67,
    USERS = 68,
    DQUOT = 69,
    STALE = 70,
    REMOTE = 71,
    BADRPC = 72,
    RPCMISMATCH = 73,
    PROGUNAVAIL = 74,
    PROGMISMATCH = 75,
    PROCUNAVAIL = 76,
    NOLCK = 77,
    NOSYS = 78,
    FTYPE = 79,
    AUTH = 80,
    NEEDAUTH = 81,
    PWROFF = 82,
    DEVERR = 83,
    OVERFLOW = 84,
    BADEXEC = 85,
    BADARCH = 86,
    SHLIBVERS = 87,
    BADMACHO = 88,
    CANCELED = 89,
    IDRM = 90,
    NOMSG = 91,
    ILSEQ = 92,
    NOATTR = 93,
    BADMSG = 94,
    MULTIHOP = 95,
    NODATA = 96,
    NOLINK = 97,
    NOSR = 98,
    NOSTR = 99,
    PROTO = 100,
    TIME = 101,
    OPNOTSUPP = 102,
    NOPOLICY = 103,
    NOTRECOVERABLE = 104,
    OWNERDEAD = 105,
    QFULL = 106,
#else
#    warning "unimplemented"
#endif
};

consteval u32 operator&(Errno&& code) { return (u32)code; }

// clang-format off
constexpr const c_string error_codes[] {
    [&Errno::NoError] = "No error",
    [&Errno::PERM] = "Operation not permitted",
    [&Errno::NOENT] = "No such file or directory",
    [&Errno::SRCH] = "No such process",
    [&Errno::INTR] = "Interrupted system call",
    [&Errno::IO] = "Input/output error",
    [&Errno::NXIO] = "Device not configured",
    [&Errno::TOOBIG] = "Argument list too long",
    [&Errno::NOEXEC] = "Exec format error",
    [&Errno::BADF] = "Bad file descriptor",
    [&Errno::CHILD] = "No child processes",
    [&Errno::DEADLK] = "Resource deadlock avoided",
    [&Errno::NOMEM] = "Cannot allocate memory",
    [&Errno::ACCES] = "Permission denied",
    [&Errno::FAULT] = "Bad address",
    [&Errno::NOTBLK] = "Block device required",
    [&Errno::BUSY] = "Device / Resource busy",
    [&Errno::EXIST] = "File exists",
    [&Errno::XDEV] = "Cross-device link",
    [&Errno::NODEV] = "Operation not supported by device",
    [&Errno::NOTDIR] = "Not a directory",
    [&Errno::ISDIR] = "Is a directory",
    [&Errno::INVAL] = "Invalid argument",
    [&Errno::NFILE] = "Too many open files in system",
    [&Errno::MFILE] = "Too many open files",
    [&Errno::NOTTY] = "Inappropriate ioctl for device",
    [&Errno::TXTBSY] = "Text file busy",
    [&Errno::FBIG] = "File too large",
    [&Errno::NOSPC] = "No space left on device",
    [&Errno::SPIPE] = "Illegal seek",
    [&Errno::ROFS] = "Read-only file system",
    [&Errno::MLINK] = "Too many links",
    [&Errno::PIPE] = "Broken pipe",
    [&Errno::DOM] = "Numerical argument out of domain",
    [&Errno::RANGE] = "Result too large",
    [&Errno::AGAIN] = "Resource temporarily unavailable",
    [&Errno::INPROGRESS] = "Operation now in progress",
    [&Errno::ALREADY] = "Operation already in progress",
    [&Errno::NOTSOCK] = "Socket operation on non-socket",
    [&Errno::DESTADDRREQ] = "Destination address required",
    [&Errno::MSGSIZE] = "Message too long",
    [&Errno::PROTOTYPE] = "Protocol wrong type for socket",
    [&Errno::NOPROTOOPT] = "Protocol not available",
    [&Errno::PROTONOSUPPORT] = "Protocol not supported",
    [&Errno::SOCKTNOSUPPORT] = "Socket type not supported",
    [&Errno::NOTSUP] = "Operation not supported",
    [&Errno::PFNOSUPPORT] = "Protocol family not supported",
    [&Errno::AFNOSUPPORT] = "Address family not supported by protocol family",
    [&Errno::ADDRINUSE] = "Address already in use",
    [&Errno::ADDRNOTAVAIL] = "Can't assign requested address",
    [&Errno::NETDOWN] = "Network is down",
    [&Errno::NETUNREACH] = "Network is unreachable",
    [&Errno::NETRESET] = "Network dropped connection on reset",
    [&Errno::CONNABORTED] = "Software caused connection abort",
    [&Errno::CONNRESET] = "Connection reset by peer",
    [&Errno::NOBUFS] = "No buffer space available",
    [&Errno::ISCONN] = "Socket is already connected",
    [&Errno::NOTCONN] = "Socket is not connected",
    [&Errno::SHUTDOWN] = "Can't send after socket shutdown",
    [&Errno::TOOMANYREFS] = "Too many references: cannot splice",
    [&Errno::TIMEDOUT] = "Operation timed out",
    [&Errno::CONNREFUSED] = "Connection refused",
    [&Errno::LOOP] = "Too many levels of symbolic links",
    [&Errno::NAMETOOLONG] = "File name too long",
    [&Errno::HOSTDOWN] = "Host is down",
    [&Errno::HOSTUNREACH] = "No route to host",
    [&Errno::NOTEMPTY] = "Directory not empty",
    [&Errno::PROCLIM] = "Too many processes",
    [&Errno::USERS] = "Too many users",
    [&Errno::DQUOT] = "Disc quota exceeded",
    [&Errno::STALE] = "Stale NFS file handle",
    [&Errno::REMOTE] = "Too many levels of remote in path",
    [&Errno::BADRPC] = "RPC struct is bad",
    [&Errno::RPCMISMATCH] = "RPC version wrong",
    [&Errno::PROGUNAVAIL] = "RPC prog. not available",
    [&Errno::PROGMISMATCH] = "Program version wrong",
    [&Errno::PROCUNAVAIL] = "Bad procedure for program",
    [&Errno::NOLCK] = "No locks available",
    [&Errno::NOSYS] = "Function not implemented",
    [&Errno::FTYPE] = "Inappropriate file type or format",
    [&Errno::AUTH] = "Authentication error",
    [&Errno::NEEDAUTH] = "Need authenticator",
    [&Errno::PWROFF] = "Device power is off",
    [&Errno::DEVERR] = "Device error, e.g. paper out",
    [&Errno::OVERFLOW] = "Value too large to be stored in data type",
    [&Errno::BADEXEC] = "Bad executable",
    [&Errno::BADARCH] = "Bad CPU type in executable",
    [&Errno::SHLIBVERS] = "Shared library version mismatch",
    [&Errno::BADMACHO] = "Malformed Macho file",
    [&Errno::CANCELED] = "Operation canceled",
    [&Errno::IDRM] = "Identifier removed",
    [&Errno::NOMSG] = "No message of desired type",
    [&Errno::ILSEQ] = "Illegal byte sequence",
    [&Errno::NOATTR] = "Attribute not found",
    [&Errno::BADMSG] = "Bad message",
    [&Errno::MULTIHOP] = "Reserved",
    [&Errno::NODATA] = "No message available on STREAM",
    [&Errno::NOLINK] = "Reserved",
    [&Errno::NOSR] = "No STREAM resources",
    [&Errno::NOSTR] = "Not a STREAM",
    [&Errno::PROTO] = "Protocol error",
    [&Errno::TIME] = "STREAM ioctl timeout",
    [&Errno::OPNOTSUPP] = "Operation not supported on socket",
    [&Errno::NOPOLICY] = "No such policy registered",
    [&Errno::NOTRECOVERABLE] = "State not recoverable",
    [&Errno::OWNERDEAD] = "Previous owner died",
    [&Errno::QFULL] = "Interface output queue is full",
};

constexpr const i32 error_codes_size
    = sizeof(error_codes) / sizeof(error_codes[0]);

}

c_string Error::errno_to_string(int code)
{
#if __linux__ || __APPLE__
    if (code < error_codes_size && error_codes[code])
        return error_codes[code];
    auto buffer = MUST(StringBuffer::create_saturated_fill(
        "sloppy coding ("sv, code, ")\0"sv));
    return buffer.leak();
#else
#    warning "unimplemented"
    return strerror(code);
#endif
}

ErrorCodes Error::s_error_codes;

}
