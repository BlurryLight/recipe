import sys
import os
# Import renderdoc if not already imported (e.g. in the UI)
if 'renderdoc' not in sys.modules and '_renderdoc' not in sys.modules:
    os.environ["PATH"] += os.pathsep + os.path.abspath(R'C:\coderepo\githubbase\renderdoc-1.26\x64\Development')
    sys.path.append(R"C:\coderepo\githubbase\renderdoc-1.26\x64\Development")  # noqa
    sys.path.append(R"C:\coderepo\githubbase\renderdoc-1.26\x64\Development\pymodules")  # noqa
    import renderdoc  # pylint: disable=import-error

# Alias renderdoc for legibility
rd = renderdoc

# Define a recursive function for iterating over actions


def AccumulateActions(Action: renderdoc.ActionDescription) -> int:
    Sum = 0
    for Child in Action.children:
        Sum += AccumulateActions(Child)
        if (Child.flags & renderdoc.ActionFlags.Drawcall) > 0:
            Sum += 1
    return Sum


def sampleCode(controller: renderdoc.ReplayController):
    SceneAction: renderdoc.ActionDescription = None
    for d in controller.GetRootActions():
        if d.GetName(controller.GetStructuredFile()) == 'Scene':
            SceneAction = d
            break
    if SceneAction is None:
        return

    # Iterate over the actions
    SceneCounter = {}
    for d in SceneAction.children:
        ActionName: str = d.GetName(controller.GetStructuredFile())
        ActionNums = AccumulateActions(d)
        if (ActionNums > 0):
            SceneCounter[ActionName] = ActionNums
        # sort SceneCounter by value
    Arr = sorted(SceneCounter.items(), key=lambda x: x[1], reverse=True)
    Total = sum(list(SceneCounter.values()))
    for item in Arr:
        print("{:50s} {:2.2%}" .format(str(item), item[1] / Total))


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
