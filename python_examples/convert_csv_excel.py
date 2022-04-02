import win32com.client as win32
from win32com.client import constants
from pathlib import Path

# wps 对应的是 kwps.Application ket.Application kwpp.Application
# office对应的是 Word.Application Excel.Application PowerPoint.Application
# excel = win32.gencache.EnsureDispatch('Excel.Application')  # office
excel = win32.gencache.EnsureDispatch('kexcel.Application')  # wps
excel.Visible = False
input_path = Path("input.csv").absolute()
output_path = Path(".").absolute()
csv = excel.Workbooks.Open(str(input_path))
csv.Activate()
csv.SaveAs(
    str(output_path/"output.xlsx"), FileFormat=constants.xlOpenXMLStrictWorkbook  # xlsx
)
csv.Close(False)
excel.Quit()
