#include <string.h>
#include <stdlib.h>
#include "Startup/app.h"
#include "Application/CmdLine.h"

struct SAllowedOptionGroup
{
	struct SCommandLineOption *psOptions;
	struct SAllowedOptionGroup *psNextLink;
};

#define	MyMalloc(x)	MemAlloc(x)
#define MyFree(x)	MemFree(*x)
#define ERROR_OUT	stderr

static struct SCmdOpt *sg_sCmds = NULL;
static struct SAllowedOptionGroup *sg_psAllowedOptionGroupHead = NULL;

/************************************************************************
 *					
 * Name : AddCmdOpt
 *					
 * Entry: Command name, and command value
 *					
 * Exit : Nothing
 *					
 * Description:
 *					
 * This routine will add in a command line option. It's OK for the command
 * value to be NULL. If the command already exists in the list, it is
 * not added. unless bReplace is set ...
 * 				
 ************************************************************************/

void AddCmdOpt(UINT8 *pu8CmdName, UINT8 *pu8CmdVal, BOOL bReplace)
{
	struct SCmdOpt *psTemp = NULL;
	struct SCmdOpt *psPrev = NULL;

	if (NULL == sg_sCmds)
	{
		sg_sCmds = MyMalloc(sizeof(*sg_sCmds));
		psTemp = sg_sCmds;
	}
	else
	{
		psTemp = sg_sCmds;
		psPrev = NULL;

		while (psTemp)
		{
			if (_stricmp((char *) psTemp->pu8CmdName, (char *) pu8CmdName) == 0)
			{
				if (bReplace)
				{
					// free old command
					if (psTemp->pu8CmdVal)
					{
						MyFree((void **) &psTemp->pu8CmdVal);
					}

					if (pu8CmdVal)
					{
						psTemp->pu8CmdVal = MyMalloc(strlen((char *) pu8CmdVal) + 1);
						strcpy((char *) psTemp->pu8CmdVal, (char *) pu8CmdVal);
					}
				}

				return;
			}

			psPrev = psTemp;
			psTemp = psTemp->psNextLink;
		}

		GCASSERT(psPrev);
		psTemp = psPrev;
		psTemp->psNextLink = MyMalloc(sizeof(*psTemp->psNextLink));
		psTemp = psTemp->psNextLink;
	}

	// We now have psTemp pointing to the new structure

	GCASSERT(pu8CmdName);
	psTemp->pu8CmdName = MyMalloc(strlen((char *) pu8CmdName) + 1);
	strcpy((char *) psTemp->pu8CmdName, (char *) pu8CmdName);

	// If our value is non-null, allocate space and copy it in!

	if (pu8CmdVal)
	{
		psTemp->pu8CmdVal = MyMalloc(strlen((char *) pu8CmdVal) + 1);
		strcpy((char *) psTemp->pu8CmdVal, (char *) pu8CmdVal);
	}
}

/************************************************************************
 *					
 * Name : DestroyOptions
 *					
 * Entry: Nothing
 *					
 * Exit : Nothing
 *					
 * Description:
 *					
 * This routine will rip through the command line options and deallocate
 * any memory allocated.
 * 				
 ************************************************************************/

void DestroyOptions(void)
{
	struct SCmdOpt *psTemp = NULL;

	psTemp = sg_sCmds;

	while (psTemp)
	{
		MyFree((void **) &psTemp->pu8CmdName);
		if (psTemp->pu8CmdVal)
			MyFree((void **) &psTemp->pu8CmdVal);
		sg_sCmds = psTemp;
		psTemp = psTemp->psNextLink;
		MyFree((void **) &sg_sCmds);
	}

	sg_sCmds = NULL;
}

/************************************************************************
 *					
 * Name : GetOption
 *					
 * Entry: Option name to search for
 *					
 * Exit : Pointer to option structure
 *					
 * Description:
 *
 * This routine will scan the existing command line option list for a
 * particular option.
 * 				
 ************************************************************************/

struct SCmdOpt *GetOption(UINT8 *pu8OptionName)
{
	struct SCmdOpt *psTemp = NULL;

	psTemp = sg_sCmds;

	while (psTemp)
	{
		if (_stricmp((char *) psTemp->pu8CmdName, (char *) pu8OptionName) == 0)
			return(psTemp);
		psTemp = psTemp->psNextLink;
	}

	return(NULL);
}

/************************************************************************
 *					
 * Name : Option
 *					
 * Entry: Option name to find
 *					
 * Exit : TRUE If the option has been given, otherwise FALSE
 *					
 * Description:
 *					
 * This routine will scan the option list to find the option given on input.
 * 				
 ************************************************************************/

BOOL Option(char *pu8OptionName)
{
	if (GetOption(pu8OptionName) == NULL)
		return(FALSE);
	else
		return(TRUE);
}


/************************************************************************
 *					
 * Name : OptionValue
 *					
 * Entry: Name of option value to get
 *					
 * Exit : Pointer to option's value
 *					
 * Description:
 *					
 * This routine will find out the value of a given command line option.
 * Be sure to call Option() first to verify that the command line option
 * has been given!
 * 				
 ************************************************************************/

char *OptionValue(char *pu8OptionName)
{
	struct SCmdOpt *psTemp = NULL;
	
	psTemp = GetOption(pu8OptionName);
	GCASSERT(psTemp);	// If this asserts, then you're looking for a command line
					// value that doesn't exist.

	return((char *)psTemp->pu8CmdVal);
}

/************************************************************************
 *					
 * Name : ProcessOptions(int argc, char **argv)
 *					
 * Entry: argc and argv from main(), with command line removed (i.e.:
 *			argc-1, argv+1
 *					
 * Exit : number of errors that occurred, or 0 if okay.
 *					
 * Description:
 *					
 * This routine runs through command line options and parses out options
 * 				
 ************************************************************************/

void ProcessOptions(int argc, char **argv)
{
	UINT32 u32Item = 0;

	// Load all of the options in to our linked list

	while (u32Item < (UINT32) argc)
	{
		AddCmdOpt((UINT8 *) argv[u32Item], NULL, 0);
		u32Item ++;
	}
}

static BOOL RescanOptions(struct SCommandLineOption *psOptions,
						  char *pu8Program)
{
	struct SCmdOpt *psTemp = sg_sCmds;
	struct SCmdOpt *psFreeMe = NULL;

	while (psTemp)
	{
		if (_stricmp((char *) psTemp->pu8CmdName, (char *) psOptions->pu8Cmd) == 0)
		{
			psTemp->psCmdLineOpt = psOptions;
		}
		
		if ( (_stricmp((char *) psTemp->pu8CmdName, (char *) psOptions->pu8Cmd) == 0) &&
			 (NULL == psTemp->pu8CmdVal) && (psOptions->bRequiresValue) )
			 
		{
			char u8String[300];

			sprintf(u8String, "Error: Command line option '%s' requires a value", psOptions->pu8Cmd);

			// A candidate for collapse! Assume the item following it is its option

			psFreeMe = psTemp->psNextLink;

			if (NULL == psTemp->psNextLink)
			{
				MessageBox(NULL,
						   u8String,
						   pu8Program,
						   MB_OK);

				return(FALSE);
			}

			// Make the next command's command line option name this command line option's value
			// and free up the old one

			GCASSERT(psOptions);
			psTemp->psCmdLineOpt = psOptions;
			psTemp->pu8CmdVal = psTemp->psNextLink->pu8CmdName;
			psTemp->psNextLink = psTemp->psNextLink->psNextLink;
			MyFree((void **) &psFreeMe);
		}

		psTemp = psTemp->psNextLink;
	}

	return(TRUE);
}

static BOOL CmdScanGroupTree(struct SCommandLineOption *psOptionList,
							 char *pu8ProgramName)
{
	BOOL bResult;

	while (psOptionList->pu8Cmd)
	{
		if (psOptionList->psAllowedIfSpecified)
		{
			bResult = CmdScanGroupTree(psOptionList->psAllowedIfSpecified,
									   pu8ProgramName);
			if (FALSE == bResult)
			{
				return(FALSE);
			}
		}

		bResult = RescanOptions(psOptionList,
								pu8ProgramName);
		if (FALSE == bResult)
		{
			return(FALSE);
		}
		psOptionList++;
	}

	return(TRUE);
}

static BOOL CmdScanAllGroups(struct SAllowedOptionGroup *psOptionGroup,
							 char *pu8ProgramName)
{
	struct SCmdOpt *psTemp = NULL;

	while (psOptionGroup)
	{
		if (FALSE == CmdScanGroupTree(psOptionGroup->psOptions,
									  pu8ProgramName))
		{
			return(FALSE);
		}

		psOptionGroup = psOptionGroup->psNextLink;
	}

	return(TRUE);
}

BOOL CmdAddAllowedTree(struct SCommandLineOption *psDefaultCommands,
					   char *pu8ProgramName)
{
	struct SAllowedOptionGroup *psOptionGroup;
	
	// If they've passed us NULL, don't do anything

	if (NULL == psDefaultCommands)
	{
		return(TRUE);
	}

	psOptionGroup = MyMalloc(sizeof(*psOptionGroup));
	psOptionGroup->psOptions = psDefaultCommands;

	if (NULL == sg_psAllowedOptionGroupHead)
	{
		sg_psAllowedOptionGroupHead = psOptionGroup;
	}
	else
	{
		psOptionGroup->psNextLink = sg_psAllowedOptionGroupHead;
		sg_psAllowedOptionGroupHead = psOptionGroup;
	}

	return(CmdScanAllGroups(psOptionGroup,
							pu8ProgramName));
}

BOOL CmdLineOptionCheck(char *pu8ProgramName)
{
	struct SCmdOpt *psTemp = sg_sCmds;

	// First check to make sure there are no missing arguments to any command
	// line options that require them

	while (psTemp)
	{
		if ( (NULL == psTemp->pu8CmdVal) &&
			 (psTemp->psCmdLineOpt) &&
			 (psTemp->psCmdLineOpt->bRequiresValue) )
		{
			char u8String[300];

			sprintf(u8String, "Error: Command line option '%s' requires a value", psTemp->pu8CmdName);
			MessageBox(NULL,
					   u8String,
					   pu8ProgramName,
					   MB_OK);
			DebugOutFunc("%s\n", u8String);
			return(FALSE);
		}

		if (NULL == psTemp->psCmdLineOpt)
		{
			char u8String[300];

			sprintf(u8String, "Error: Command line option '%s' unknown", psTemp->pu8CmdName);
			MessageBox(NULL,
					   u8String,
					   pu8ProgramName,
					   MB_OK);
			DebugOutFunc("%s\n", u8String);
			return(FALSE);
		}

		psTemp = psTemp->psNextLink;
	}

	return(TRUE);
}

BOOL CmdLineInit(char *pu8CmdLine,
				 SCommandLineOption *psOptions,
				 char *pu8ProgramName)
{
	char **argv;
	LPWSTR *argvW;
	int argc;
	UINT32 u32Loop;

	argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
	argv = calloc(sizeof(*argv) * argc, 1);
	GCASSERT(argv);

	for (u32Loop = 0; u32Loop < (UINT32) argc; u32Loop++)
	{
		argv[u32Loop] = calloc(wcslen(argvW[u32Loop]) + 1, 1);
		GCASSERT(argv[u32Loop]);
		wcstombs(argv[u32Loop], argvW[u32Loop], wcslen(argvW[u32Loop]));
	}

	ProcessOptions(argc - 1, argv + 1);
	return(CmdAddAllowedTree(psOptions,
							 pu8ProgramName));
}

BOOL CmdLineInitArgcArgv(int argc,
						 char **argv,
						 SCommandLineOption *psOptions,
						 char *pu8ProgramName)
{
	ProcessOptions(argc - 1, argv + 1);
	return(CmdAddAllowedTree(psOptions,
							 pu8ProgramName));
}
				