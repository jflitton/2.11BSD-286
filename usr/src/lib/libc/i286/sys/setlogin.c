/*
 * setlogin(name): getlogin() keeps the name once it has asked, and must
 * ask again after this.
 */
extern int _logname_valid;

setlogin(name)
	char *name;
{
	_logname_valid = 0;
	return (_setlogin(name));
}
