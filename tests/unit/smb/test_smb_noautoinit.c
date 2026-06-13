/*******************************************************************************
 * Override the library's weak SMB constructor/destructor to prevent auto-init
 * from binding UDP broadcast ports during unit tests.
 * Compile and link this into every SMB unit test binary.
 ******************************************************************************/

void init_smb(void)      { /* noop — suppressed for tests */ }
void terminate_smb(void) { /* noop — suppressed for tests */ }
