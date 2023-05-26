import re
import sys
import os
from itertools import accumulate
# Import renderdoc if not already imported (e.g. in the UI)
if 'renderdoc' not in sys.modules and '_renderdoc' not in sys.modules:
    os.environ["PATH"] += os.pathsep + os.path.abspath(R'C:\coderepo\githubbase\renderdoc-1.26\x64\Development')
    sys.path.append(R"C:\coderepo\githubbase\renderdoc-1.26\x64\Development")  # noqa
    sys.path.append(R"C:\coderepo\githubbase\renderdoc-1.26\x64\Development\pymodules")  # noqa
    import renderdoc  # pylint: disable=import-error

# Alias renderdoc for legibility
rd = renderdoc

# Define a recursive function for iterating over actions
pattern = "IndirectDraw\(<\d+,\s*(\d+)>\)"
prog = re.compile(pattern)


def accumulateActions(controller: renderdoc.ReplayController, Action: renderdoc.ActionDescription, FilterZero=False) -> int:
    Sum = 0
    for Child in Action.children:
        Sum += accumulateActions(controller, Child, FilterZero)
        if (Child.flags & renderdoc.ActionFlags.Drawcall) == 0:
            continue
        if (FilterZero):
            ChildName = Child.GetName(controller.GetStructuredFile())
            m = prog.search(ChildName)
            if (m is not None and int(m.group(1)) == 0):
                continue
        Sum += 1
    return Sum


def locateAction(controller, Action: renderdoc.ActionDescription, name: str) -> renderdoc.ActionDescription:
    if Action.GetName(controller.GetStructuredFile()) == name:
        return Action
    for Child in Action.children:
        Result = locateAction(controller, Child, name)
        if Result is not None:
            return Result
    return None


def iterScene(controller: renderdoc.ReplayController, SceneAction: renderdoc.ActionDescription, FilterZero):
    print("Iter SceneAction, Filtering Zero {}", FilterZero)
    # Iterate over the actions
    SceneCounter = {}
    for d in SceneAction.children:
        ActionName: str = d.GetName(controller.GetStructuredFile())
        ActionNums = accumulateActions(controller, d, FilterZero=FilterZero)
        if (ActionNums > 0):
            SceneCounter[ActionName] = ActionNums
        # sort SceneCounter by value
    OrderedArr = sorted(SceneCounter.items(), key=lambda x: x[1], reverse=True)
    Total = sum(list(SceneCounter.values()))
    PrefixSum = list(accumulate([item[1] for item in OrderedArr]))
    print("{:50s} {:>10s} {:>10s} {:>10s}" .format("PassName/DC", "DC%", "PrefixSum", "PrefixSum%"))
    for i in range(len(OrderedArr)):
        PassName, DC = OrderedArr[i]
        print("{:50s} {:2.2%} {} {:2.2%}" .format(OrderedArr[i], DC / Total, PrefixSum[i], PrefixSum[i] / Total))
    print("TotalDC: ", Total)


def sampleCode(controller: renderdoc.ReplayController):
    SceneAction: renderdoc.ActionDescription = None
    for d in controller.GetRootActions():
        SceneAction = locateAction(controller, d, "Scene")
        if (SceneAction):
            break

    if SceneAction is None:
        return

    iterScene(controller, SceneAction, True)
    iterScene(controller, SceneAction, False)


def loadCapture(filename):
    # Open a capture file handle
    cap = rd.OpenCaptureFile()

    # Open a particular file - see also OpenBuffer to load from memory
    result = cap.OpenFile(filename, '', None)

    # Make sure the file opened successfully
    if result != rd.ResultCode.Succeeded:
        raise RuntimeError("Couldn't open file: " + str(result))

    # Make sure we can replay
    if not cap.LocalReplaySupport():
        raise RuntimeError("Capture cannot be replayed")

    # Initialise the replay
    result, controller = cap.OpenCapture(rd.ReplayOptions(), None)

    if result != rd.ResultCode.Succeeded:
        raise RuntimeError("Couldn't initialise replay: " + str(result))

    return cap, controller


if 'pyrenderdoc' in globals():
    pyrenderdoc.Replay().BlockInvoke(sampleCode)
else:
    rd.InitialiseReplay(rd.GlobalEnvironment(), [])

    if len(sys.argv) <= 1:
        print('Usage: python3 {} filename.rdc'.format(sys.argv[0]))
        sys.exit(0)

    cap, controller = loadCapture(sys.argv[1])

    sampleCode(controller)

    controller.Shutdown()
    cap.Shutdown()

    rd.ShutdownReplay()
