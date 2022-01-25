// System error codes
//mxDECLARE_ERROR_CODE( ENUM, NUMBER, STRING )

// Success codes:
// the function returned no error, all is well.
mxDECLARE_ERROR_CODE( ALL_OK,		0, "No errors, all is well" )

// Failure codes:

// general errors
mxDECLARE_ERROR_CODE( ERR_NOT_IMPLEMENTED,			2,		"Not implemented" )
mxDECLARE_ERROR_CODE( ERR_UNSUPPORTED_FEATURE,		3,		"Unsupported feature" )
mxDECLARE_ERROR_CODE( ERR_UNSUPPORTED_VERSION,		5,		"Unsupported version" )// File version not supported (anymore or yet)
mxDECLARE_ERROR_CODE( ERR_INCOMPATIBLE_VERSION,		4,		"Incompatible version" )
mxDECLARE_ERROR_CODE( ERR_FEATURE_NOT_AVAILABLE,	6,		"Feature not available" )// The function could not complete because it tries to use a feature (or set of features) not currently supported.

// general errors
mxDECLARE_ERROR_CODE( ERR_INVALID_PARAMETER, 		50,		"Invalid argument" )	// An invalid parameter was passed to the returning function.
mxDECLARE_ERROR_CODE( ERR_INDEX_OUT_OF_RANGLE, 		51,		"Index out of range" )	// Index value outside the valid range
mxDECLARE_ERROR_CODE( ERR_INVALID_FUNCTION_CALL, 	52,		"Invalid function call" )
mxDECLARE_ERROR_CODE( ERR_NULL_POINTER_PASSED, 		53,		"Null pointer argument" )
mxDECLARE_ERROR_CODE( ERR_FAILED_TO_LOAD_LIBRARY, 	54,		"Failed to load dynamic library" )
mxDECLARE_ERROR_CODE( ERR_FAILED_TO_CREATE_THREAD, 	55,		"Failed to create thread" )	// The system does not have enough resources to create a new thread.
mxDECLARE_ERROR_CODE( ERR_SYSTEM_IS_BUSY,			56,		"The object is busy" )	// E.g. the system/task scheduler/server is busy.
mxDECLARE_ERROR_CODE( ERR_NEED_TO_WAIT,				57,		"Too little time has passed since the last call" )
mxDECLARE_ERROR_CODE( ERR_ASSERTION_FAILED, 		70,		"Assertion failed" )

// Object-related
mxDECLARE_ERROR_CODE( ERR_OBJECT_NOT_FOUND, 		100,		"Object not found" )
mxDECLARE_ERROR_CODE( ERR_OBJECT_OF_WRONG_TYPE, 	101,		"Object of wrong type" )
mxDECLARE_ERROR_CODE( ERR_NAME_ALREADY_TAKEN, 		102,		"Object with the given name already exists" )
mxDECLARE_ERROR_CODE( ERR_TOO_MANY_OBJECTS, 		103,		"There are too many unique instances of a particular type of object" )
mxDECLARE_ERROR_CODE( ERR_DUPLICATE_OBJECT, 		104,		"Such object already exists" )	// e.g. adding a duplicate key to a set of unique keys

// memory errors
mxDECLARE_ERROR_CODE( ERR_OUT_OF_MEMORY, 			150,		"Out of memory" )//Operation failed due to insufficient memory.
mxDECLARE_ERROR_CODE( ERR_STACK_OVERFLOW, 			151,		"Stack overflow" )
mxDECLARE_ERROR_CODE( ERR_STACK_UNDERFLOW, 			152,		"Stack underflow" )
mxDECLARE_ERROR_CODE( ERR_BUFFER_TOO_SMALL, 		153,		"Buffer too small" )//e.g. not enough space in a preallocated array, closed hash table too small
mxDECLARE_ERROR_CODE( ERR_INVALID_ALIGNMENT, 		154,		"Invalid alignment" )

// I/O errors
mxDECLARE_ERROR_CODE( ERR_FAILED_TO_PARSE_DATA, 	200,		"Failed to parse data" )
mxDECLARE_ERROR_CODE( ERR_FAILED_TO_CREATE_FILE, 	201,		"Failed to create file" )
mxDECLARE_ERROR_CODE( ERR_FAILED_TO_OPEN_FILE, 		202,		"Failed to open file" )
mxDECLARE_ERROR_CODE( ERR_FAILED_TO_READ_FILE,		203,		"Failed to read file" )
mxDECLARE_ERROR_CODE( ERR_FAILED_TO_WRITE_FILE, 	204,		"Failed to write file" )
mxDECLARE_ERROR_CODE( ERR_FAILED_TO_SEEK_FILE, 		205,		"Failed to seek file" )
mxDECLARE_ERROR_CODE( ERR_FILE_HAS_ZERO_SIZE, 		206,		"File has zero size" )
mxDECLARE_ERROR_CODE( ERR_FILE_OR_PATH_NOT_FOUND, 	207,		"File or path not found" )
mxDECLARE_ERROR_CODE( ERR_FAILED_TO_LOAD_DATA, 		208,		"Failed to load data" )

// Miscellaneous errors
mxDECLARE_ERROR_CODE( ERR_ABORTED_BY_USER,			300,		"Aborted by the user" )//Long-running task cancelled by the user.
//-- Parsing/Scripts
mxDECLARE_ERROR_CODE( ERR_EOF_REACHED,				400,		"End of file is reached" )
mxDECLARE_ERROR_CODE( ERR_MISSING_NAME,				499,		"Missing name" )
mxDECLARE_ERROR_CODE( ERR_UNEXPECTED_TOKEN,			500,		"Unexpected token" )
mxDECLARE_ERROR_CODE( ERR_SYNTAX_ERROR, 			501,		"Syntax error" )
mxDECLARE_ERROR_CODE( ERR_COMPILATION_FAILED, 		502,		"Compilation failed" )
mxDECLARE_ERROR_CODE( ERR_LINKING_FAILED, 			503,		"Linking failed" )
mxDECLARE_ERROR_CODE( ERR_VALIDATION_FAILED, 		504,		"Validation failed" )
mxDECLARE_ERROR_CODE( ERR_RUNTIME_ERROR, 			505,		"Run-time error" )
mxDECLARE_ERROR_CODE( ERR_WHILE_ERROR_HANDLER, 		506,		"Error while running the error handler" )

//-- Graphics errors
mxDECLARE_ERROR_CODE( ERR_FAILED_TO_CREATE_BUFFER, 	700,		"Failed to create buffer" )

//-- Authentication
mxDECLARE_ERROR_CODE( ERR_WRONG_PASSWORD, 			900,		"Wrong password" )

//-- Math errors
mxDECLARE_ERROR_CODE( ERR_DIVISION_BY_ZERO, 		1000,		"Division by Zero!" )

//-- Voxel Engine errors
mxDECLARE_ERROR_CODE( ERR_CHUNK_CANNOT_BE_EDITED, 	2000,		"Chunk cannot be edited" )
mxDECLARE_ERROR_CODE( ERR_ALREADY_EDITING,			2001,		"Tried to edit the world that that was already being edited" )

//mxDECLARE_ERROR_CODE( ERR_FAILED_TO_LOAD_ASSET, 	XXX,		"Asset not found" )

// NOTE: must always be last! see EReturnCode_To_Chars()
// NOTE: avoid using this code, always try to give meaningful error info.
mxDECLARE_ERROR_CODE( ERR_UNKNOWN_ERROR, 			1,		"Unknown error" )
