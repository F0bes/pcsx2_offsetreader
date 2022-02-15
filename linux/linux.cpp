// Ty (Fobes):
// Some sample code to show how you can read the recompiler base address from pcsx2 in linux
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>

#include <sys/uio.h>

int main(void)
{
	FILE *fp;

	printf("Finding the pid of pcsx2\n");
	char pidofOutChar[255];
	memset(pidofOutChar, 0, 255);
	fp = popen("pidof pcsx2", "r");

	fgets(pidofOutChar, 255, fp);
	pid_t pid = strtoul(pidofOutChar, 0, 10);

	pclose(fp);

	if (pid == 0)
	{
		printf("pcsx2 is not running?\n");
		return 0;
	}

	printf("pcsx2 pid: %d\n", pid);

	printf("Reading symbols from pcsx2(%d)\n", pid);
	std::string cmd = "nm /proc/" + std::to_string(pid) + "/exe | grep EEmem";
	fp = popen(cmd.c_str(), "r");

	char nmOutChar[255];
	memset(nmOutChar, 0, 255);
	fgets(nmOutChar, 255, fp);
	pclose(fp);

	std::string nmOut(nmOutChar);
	if(nmOut.empty())
	{
		printf("Error: Could not find EEmem in nm output\n Are you running a proper version of pcsx2? Are you root?\n PR #5531 introduced this feature.\n");
		return -1;
	}

	printf("NM output -> %s\n", nmOut.c_str());
	nmOut = nmOut.substr(0, nmOut.find(" "));
	printf("Relative address -> %s\n", nmOut.c_str());
	
	// nmOut is the address of eeMem relative to where pcsx2 is loaded
	// Now get where pcsx2 is loaded

	printf("Reading symbols from /proc/%d/maps to get the current pcsx2 base address\n", pid);
	cmd = "cat /proc/" + std::to_string(pid) + "/maps | grep pcsx2";
	fp = popen(cmd.c_str(), "r");

	char mapsOutChar[1024];
	memset(mapsOutChar, 0, 1024);
	fgets(mapsOutChar, 1024, fp);
	pclose(fp);
	std::string mapsOut(mapsOutChar);
	mapsOut = mapsOut.substr(0, mapsOut.find("-"));
	printf("Maps output -> %s\n", mapsOut.c_str());

	unsigned long targetAddress = strtoul(nmOut.c_str(), 0, 16) + strtoul(mapsOut.c_str(), 0, 16);

	printf("Target address -> %lx\n", targetAddress);
	struct iovec local[1];
	local[0].iov_base = calloc(8, sizeof(char));
	local[0].iov_len = 8;

	struct iovec remote[1];
	remote[0].iov_base = (void*)targetAddress;
	remote[0].iov_len = 8;

	if(process_vm_readv(pid, local, 1, remote, 1, 0) < 0)
	{
		printf("Error: Could not read from process\n");
		if(errno == EPERM)
		{
			printf("Error: You do not have permission to read from this process, run as root\n");
		}

		printf("strerror: %s\n", strerror(errno));
		return -1;
	}

	printf("EEmem -> %lx\n", *(unsigned long*)local[0].iov_base);
	return 0;
}