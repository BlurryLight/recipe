import re
import sys
import os
from itertools import accumulate
from typing import List
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

bPrintShadowInfo = False
bPrintBasicInfo = True
bPrintBasepassFoliageInfo = True


def accumulateDrawCall(controller: renderdoc.ReplayController, Action: renderdoc.ActionDescription, FilterZero=False) -> int:
    Sum = 0
    for Child in Action.children:
        Sum += accumulateDrawCall(controller, Child, FilterZero)
        if (Child.flags & renderdoc.ActionFlags.Drawcall) == 0:
            continue
        if (FilterZero):
            ChildName = Child.GetName(controller.GetStructuredFile())
            m = prog.search(ChildName)
            if (m is not None and int(m.group(1)) == 0):
                continue
        Sum += 1
    return Sum


def accumulateEstimatedVertices(controller: renderdoc.ReplayController, Action: renderdoc.ActionDescription) -> int:
    Sum = 0
    # ActionName = Action.GetName(controller.GetStructuredFile())
    # print(ActionName, f"indices: {Action.numIndices}", f"instances: {Action.numInstances}")
    for Child in Action.children:
        Sum += accumulateEstimatedVertices(controller, Child)
        if (Child.flags & renderdoc.ActionFlags.Drawcall) == 0:
            continue
        # ChildName = Child.GetName(controller.GetStructuredFile())
        # print(ChildName, f"indices: {Child.numIndices}", f"instances: {Child.numInstances}")
        Sum += Child.numIndices * Child.numInstances
    return Sum

# accurate locate only one


def locateAction(controller, Action: renderdoc.ActionDescription, name: str) -> renderdoc.ActionDescription:
    if Action.GetName(controller.GetStructuredFile()) == name:
        return Action
    for Child in Action.children:
        Result = locateAction(controller, Child, name)
        if Result is not None:
            return Result
    return None

# dfs search Action has substring


def searchAction(controller, BeginAction: renderdoc.ActionDescription, Name: str, ResActions: List[renderdoc.ActionDescription], depth, maxDepth=10):
    if (depth >= maxDepth):  # limit dfs max depth
        return
    if BeginAction.GetName(controller.GetStructuredFile()).find(Name) != -1:
        ResActions.append(BeginAction)
    for Child in BeginAction.children:
        searchAction(controller, Child, Name, ResActions, depth + 1, maxDepth)


def iterateShadowDepths(controller: renderdoc.ReplayController, Action: renderdoc.ActionDescription, FilterZero=False):
    print(f"Iter Shadows, Filtering Zero {FilterZero}")
    SplitActions: List[renderdoc.ActionDescription] = []
    searchAction(controller, Action, "WholeScene split", SplitActions, depth=0, maxDepth=10)
    print("Cascade Nums: ", len(SplitActions))
    for x in SplitActions:
        ActionName: str = x.GetName(controller.GetStructuredFile())
        ActionNums = accumulateDrawCall(controller, x, FilterZero=FilterZero)
        print("{:50s} {:>10s}" .format(ActionName, str(ActionNums)))


def iterateBasePassForName(controller: renderdoc.ReplayController, Name, Action: renderdoc.ActionDescription, FilterZero=False):
    print(f"Iter BasePass {Name}, Filtering Zero {FilterZero}")
    TargetActions: List[renderdoc.ActionDescription] = []
    searchAction(controller, Action, Name, TargetActions, depth=0, maxDepth=3)
    Total = 0
    for x in TargetActions:
        ActionName: str = x.GetName(controller.GetStructuredFile())
        ActionNums = accumulateDrawCall(controller, x, FilterZero=FilterZero)
        if (ActionNums > 0):
            print("{:50s} {:>10s}" .format(ActionName, str(ActionNums)))
            Total += 1
    print("Total DC:", Total)


def iterateBasePassFoliage(controller: renderdoc.ReplayController, Action: renderdoc.ActionDescription, FilterZero=False):
    iterateBasePassForName(controller, "Foliage", Action, FilterZero)


def iterSceneDrawCall(controller: renderdoc.ReplayController, SceneAction: renderdoc.ActionDescription, FilterZero):
    print(f"Iter SceneAction, Filtering Zero {FilterZero}")
    # Iterate over the actions
    SceneDCCounter = {}
    SceneVerticesCounter = {}
    for d in SceneAction.children:
        ActionName: str = d.GetName(controller.GetStructuredFile())
        ActionDCNums = accumulateDrawCall(controller, d, FilterZero=FilterZero)
        if (ActionDCNums > 0):
            SceneDCCounter[ActionName] = ActionDCNums

        ActionVerticesNums = accumulateEstimatedVertices(controller, d)
        if (ActionVerticesNums > 0):
            SceneVerticesCounter[ActionName] = ActionVerticesNums

    OrderedArr = sorted(SceneDCCounter.items(), key=lambda x: x[1], reverse=True)
    Total = sum(list(SceneDCCounter.values()))
    PrefixSum = list(accumulate([item[1] for item in OrderedArr]))
    print("{:50s} {:>10s} {:>10s} {:>10s} {:>10s}" .format("PassName/DC", "DC%", "PrefixSum", "PrefixSum%", "Vertices"))
    for i in range(len(OrderedArr)):
        PassName, DC = OrderedArr[i]
        print("{:50s} {:2.2%} {} {:2.2%} {}" .format(
            str(OrderedArr[i]), DC / Total, PrefixSum[i], PrefixSum[i] / Total, SceneVerticesCounter[PassName]))
    print("TotalDC: ", Total)


def sampleCode(controller: renderdoc.ReplayController):
    SceneAction: renderdoc.ActionDescription = None
    for d in controller.GetRootActions():
        SceneAction = locateAction(controller, d, "Scene")
        if (SceneAction):
            break

    if SceneAction is None:
        return

    if bPrintBasicInfo:
        iterSceneDrawCall(controller, SceneAction, True)
        print("\n" * 3)
        iterSceneDrawCall(controller, SceneAction, False)

    if (bPrintShadowInfo):
        ShadowDepthsAction: renderdoc.ActionDescription = None
        ShadowDepthsAction = locateAction(controller, SceneAction, "ShadowDepths")
        if (ShadowDepthsAction):
            iterateShadowDepths(controller, ShadowDepthsAction, FilterZero=True)
            print("\n" * 3)
            iterateShadowDepths(controller, ShadowDepthsAction, FilterZero=False)

    if (bPrintBasepassFoliageInfo):
        BasepassAction = locateAction(controller, SceneAction, "BasePassParallel")  # dx12 only
        if (BasepassAction):
            iterateBasePassFoliage(controller, BasepassAction, FilterZero=True)
            print("\n" * 3)
            iterateBasePassFoliage(controller, BasepassAction, FilterZero=False)


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
