rowData = [[0 for x in range(7)] for y in range(3)]
fileOut = open("mock_parameters_modulation.map","w")

fileOut.write("!modulation control parameters\n")
fileOut.write("!parameter_type,\tbmodWinPerPeriod,\tbmodPeriodsPerCoil,\tbmodNCoils,\tbmodCyclesperSuper,\tbmodIdlePerCycle, \tbmodIdlePerSuper\n")
fileOut.write("!default parameters if none are provided are: 8, 5, 7, 6, 40, 80\n")
fileOut.write("bmodcontrolpar, 8, 5, 7, 6, 40, 80\n")
fileOut.write("\n")

fileOut.write("!coil amplitude\n")
fileOut.write("!device_type,\tdevice_name,\tbmmodCoilAmplitude\n")
fileOut.write("coilAmp,\tcoil1,\t500\n")
fileOut.write("coilAmp,\tcoil2,\t500\n")
fileOut.write("coilAmp,\tcoil3,\t500\n")
fileOut.write("coilAmp,\tcoil4,\t500\n")
fileOut.write("coilAmp,\tcoil5,\t500\n")
fileOut.write("coilAmp,\tcoil6,\t500\n")
fileOut.write("coilAmp,\tcoil7,\t150\n")
fileOut.write("\n")

fileOut.write("!Target BPM Response to trim card values\n")
fileOut.write("!device_type,\tdevice_name,\tX,\tY,\tY,\tY',\tE\n")
fileOut.write("bmodtargetresponse,\tbmod_trim1,\t-1.00,\t0.100,\t0.800,\t0.05,\t0.000\n")
fileOut.write("bmodtargetresponse,\tbmod_trim2,\t0.150,\t1.000,\t0.100,\t-0.90,\t0.000\n")
fileOut.write("bmodtargetresponse,\tbmod_trim3,\t-0.80,\t-0.15,\t-1.00,\t0.10,\t0.000\n")
fileOut.write("bmodtargetresponse,\tbmod_trim4,\t0.050,\t0.900,\t0.050,\t1.00,\t0.000\n")
fileOut.write("bmodtargetresponse,\tbmod_trim5,\t1.200,\t0.200,\t0.500,\t0.10,\t0.000\n")
fileOut.write("bmodtargetresponse,\tbmod_trim6,\t0.200,\t-1.10,\t0.100,\t0.60,\t0.000\n")
fileOut.write("bmodtargetresponse,\tbmod_trim7,\t0.600,\t0.050,\t0.300,\t0.05,\t0.000\n")
fileOut.write("\n")

fileOut.write("!reverse transfer matrices from RamRM_IMOLLER_to_IPM1C01.txt\n")
fileOut.write("!parameter_type,\tdevice_name,\trow_num, \tcxx,\tcxxp,\tcxy,\tcxyp,\tcxe,\tcyx,\tcyxp,\tcyy,\tcyyp,\tcye\n")
with open("RanTM_IMOLLER_to_IPM1C01.txt", "r") as f:
    for line in f:
        tokens = line.split()
        if line.__contains__("Element"):
            for token in tokens:
                if token.__contains__("IPM"):
                    bpmName = (token.replace("IPM", "bpm")).lower()
                    #print(bpmName)
        elif line.__contains__("R1"):
            for i in range(7):
                #print(tokens[i])
                rowData[0][i] = tokens[i]
            fileOut.write("bpmtransfermatrix,\t"+bpmName+',\t'+rowData[0][0][:-1]+',\t'+rowData[0][1]+',\t'+rowData[0][2]+',\t'+rowData[0][3]+',\t'+rowData[0][4]+',\t'+rowData[0][6]+'\n')
        elif line.__contains__("R3"):
            for i in range(7):
                rowData[1][i] = tokens[i]
            fileOut.write("bpmtransfermatrix,\t"+bpmName+',\t'+rowData[1][0][:-1]+',\t'+rowData[1][1]+',\t'+rowData[1][2]+',\t'+rowData[1][3]+',\t'+rowData[1][4]+',\t'+rowData[1][6]+'\n')
fileOut.write("\n")
fileOut.write("!Forward Transport Matrices from BeamMod_at_Target.txt\n")
fileOut.write("!parameter_type,\tdevice_name,\trow_num, \tcxx,\tcxxp,\tcxy,\tcxyp,\tcxe,\tcyx,\tcyxp,\tcyy,\tcyyp,\tcye\n")
with open("BeamMod_at_Target.txt", "r") as f:
    for line in f:
        tokens = line.split()
        if line.__contains__("matrix"):
            for token in tokens:
                if token.__contains__("MAT"):
                    bpmName = (token.replace("MAT", "bpm")).lower()
                    #print(bpmName)
        elif line.__contains__("R1"):
            for i in range(7):
                # print(tokens[i])
                rowData[0][i] = tokens[i]
            fileOut.write("bpmtransfermatrix,\t" + bpmName + ',\t' + rowData[0][0][:-1] + ',\t' + rowData[0][1] + ',\t' +rowData[0][2] + ',\t' + rowData[0][3] + ',\t' + rowData[0][4] + ',\t' + rowData[0][6] + '\n')
        elif line.__contains__("R3"):
            for i in range(7):
                rowData[1][i] = tokens[i]
            fileOut.write("bpmtransfermatrix,\t" + bpmName + ',\t' + rowData[1][0][:-1] + ',\t' + rowData[1][1] + ',\t' +rowData[1][2] + ',\t' + rowData[1][3] + ',\t' + rowData[1][4] + ',\t' + rowData[1][6] + '\n')