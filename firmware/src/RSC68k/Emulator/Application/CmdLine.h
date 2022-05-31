#ifndef _CMDLINE_H_
#define _CMDLINE_H_

// Internally used command option linked list structure
struct SCmdOpt
{
	char *pu8CmdName;
	char *pu8CmdVal;
	struct SCommandLineOption *psCmdLineOpt;
	struct SCmdOpt *psNextLink;
};

typedef struct SCommandLineOption
{
	const char *pu8Cmd;			// The command itself
	BOOL bRequiredOption;		// Is this option REQUIRED?
	BOOL bRequiresValue;		// Does this option require a value?
	UINT8 *pu8Description;		// Description of the command line option

	struct SCommandLineOption *psAllowedIfSpecified;
} SCommandLineOption;

extern void AddCmdOpt(UINT8 *pu8CmdName, UINT8 *pu8CmdVal, BOOL bReplace);
extern void DestroyOptions(void);
extern struct SCmdOpt *GetOption(UINT8 *pu8OptionName);
extern BOOL Option(char *pu8OptionName);
extern char *OptionValue(char *pu8OptionName);
extern struct sCommandLineOption *CheckOption(UINT8 *pu8Option);
extern BOOL CmdAddAllowedTree(struct SCommandLineOption *psDefaultCommands,
							  char *pu8ProgramName);
extern BOOL CmdLineOptionCheck(char *pu8ProgramName);
extern BOOL CmdLineInit(char *pu8CmdLine,
						SCommandLineOption *psValidOptions,
						char *pu8ProgramName);
extern BOOL CmdLineInitArgcArgv(int argc,
								char **argv,
								SCommandLineOption *psOptions,
								char *pu8ProgramName);

#endif