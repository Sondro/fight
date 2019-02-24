#define Assert(e) if(!(e)) { _AssertionFailure(__LINE__, __FILE__, #e); }
#define Log(...) fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n");

internal void
_AssertionFailure(int line, const char *file, const char *expression)
{
    platform->OutputError("Fatal Error", "Assertion of %s at %s:%i failed.", expression, file, line);
    *(int *)0 = 0;
}