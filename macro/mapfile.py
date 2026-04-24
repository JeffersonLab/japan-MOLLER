"""
     Author: Joachim Tsakanikas
    Created: 2024/07/07

Last editor: Joachim Tsakanikas
Last edited: 2024/07/30

"""

import csv
import math
import os

milli = 1e-3
micro = 1e-6

current = 100  # 100 uA
num_samples = 15_000
time = 2 * milli # seconds


class Pedestal:
    """
    Read and modify pedestal*.map files for japan-MOLLER
    """

    def __init__(self, filepath: str = "/home/joachim/my-japan-MOLLER/Parity/prminput/mock_parameters_thinqtz.map"):
        if len(filepath) < 5:
            raise ValueError(
                "Invalid path length. Path must include .map extension. E.g., '/path/to/file.map'"
            )
        if filepath[-4:] != ".map":
            raise ValueError(
                "The file must have a .map extension. E.g., '/path/to/file.map'"
            )
        if not os.path.isfile(filepath):
            raise ValueError("This path does not point to an existing file.")

        self.cols = {}  # create dictionary to easily reference columns TODO change to list
        self.entries = []  # create 2d list to contain parameters/data
        self.headers = []  # to contain file headers
        self.name: str  # to be name of file
        self.names = []  # to contain names of detectors
        self.path = ""
        self.rows = {}  # create dictionary to easily reference rows TODO change to list

        # read in file name
        i = -5
        while True:
            char = filepath[i]
            if char == "/":
                i += 1  # need to go back one step
                self.name = filepath[len(filepath) + i: -4]
                self.path = filepath[: len(filepath) + i]
                break
            if abs(i) == len(filepath):  # no directory in path
                self.name = filepath[:-4]
                break
            if abs(i) > len(filepath):
                raise RuntimeError(
                    "An unexpected error occurred while reading the file name."
                )
            i -= 1

        with open(filepath, encoding="ascii") as f:  # file is ascii encoded
            # read in headers
            headers = f.readline().split()
            if headers[0] == "!":
                headers = headers[1:]
            if headers[0] == "Name":
                headers = headers[1:]
            self.headers = headers
            # TODO self.cols = enumerate(headers)
            for index, val in enumerate(headers):
                self.cols.update({val: index})

            # read remaining lines in file and populate `rows` and `entries`
            n = 0  # create auxiliary counter
            line = f.readline()  # reads next line
            while line != "":  # readline returns '' when no lines are left
                record = line.split()  # "Hello World" -> ["Hello", "World"]
                self.rows.update({record[0]: n})  # {"Hello": 0}
                self.names.append(record[0])  # ["Hello"]
                self.entries.append(record[1:])  # ["World",...]
                line = f.readline()
                n += 1

        print(
            f"Pedestal object successfully created. {self.countrows()} rows were added.")

    def countcols(self):
        "Returns number of columns"
        return len(self.cols)

    def countrows(self):
        "Returns number of rows"
        return len(self.rows)

    def gain(self, row: str) -> float:
        "Get gain for specific detector."
        # [gain] = Volt/ADC_count
        return float(self.entries[self.rows[row]][self.cols['Gain']])

    def help(self):  # TODO
        "Is there a manual for this thing?"
        print("help() method not yet implemented")

    def hwsum(self, row: str) -> tuple[float, float]:
        "Calculate the mean and stddev of the hardware sum."
# void QwIntegrationPMT::RandomizeMollerEvent(int helicity, const QwBeamCharge& charge, const QwBeamPosition& xpos, const QwBeamPosition& ypos, const QwBeamAngle& xprime, const QwBeamAngle& yprime, const QwBeamEnergy& energy)
# {
#   QwMollerADC_Channel temp(this->fTriumf_ADC);
#   fTriumf_ADC.ClearEventData();

#   temp.AssignScaledValue(xpos, fCoeff_x);
#   fTriumf_ADC += temp;

#   temp.AssignScaledValue(ypos, fCoeff_y);
#   fTriumf_ADC += temp;

#   temp.AssignScaledValue(xprime, fCoeff_xp);
#   fTriumf_ADC += temp;

#   temp.AssignScaledValue(yprime, fCoeff_yp);
#   fTriumf_ADC += temp;

#   temp.AssignScaledValue(energy, fCoeff_e);
#   fTriumf_ADC += temp;

# //fTriumf_ADC.AddChannelOffset(fBaseRate * (1+helicity*fAsym));
#   fTriumf_ADC.AddChannelOffset(1.0+helicity*fAsym);

#   fTriumf_ADC *= charge;
#   fTriumf_ADC.Scale(fNormRate*fVoltPerHz);  //  After this Scale function, fTriumf_ADC should be the detector signal in volts.
#   fTriumf_ADC.ForceMapfileSampleSize();
#   //  Double_t voltage_width = sqrt(fTriumf_ADC.GetValue()*window_length/fVoltPerHz)/(window_length/fVoltPerHz);
#   Double_t voltage_width = sqrt( fTriumf_ADC.GetValue() / (fTriumf_ADC.GetNumberOfSamples()*QwMollerADC_Channel::kTimePerSample/Qw::sec/fVoltPerHz) );
#   //std::cout << "Voltage Width: " << voltage_width << std::endl;
#   fTriumf_ADC.SmearByResolution(voltage_width);
#   fTriumf_ADC.SetRawEventData();

#   return;

# }
        # hw_sum is "charge normalized" and has units of V
        # * (current / gain) * (gain / current)
        # Standard deviation calculated in QwIntegrationPMT.cc on line 146:
        # `voltage_width = sqrt(fTriumf_ADC.GetValue() * window_length / VoltPerHz) * (VoltPerHz / window_length)`
        mean = self.voltperhz(row) * self.normrate(row) * current
        stddev = math.sqrt(num_samples) * self.voltperhz(row) / time
        return (mean, stddev)

    # def hwsumraw(self, row: str) -> tuple[float, float]:
    #     num_samples: int = 15000
    #     mean = self.voltperhz(row) * self.normrate(row) * num_samples
    #     return (mean, stddev)

    def import_tq_rates(self, filepath: str) -> None:
        """
        Import thin quartz detector rates from Dr. Paschke's CSV file.

        Parameters
        ----------
        filepath: str
                  Path to file including file name and extension,
                  e.g., "path/to/file.csv".

        Notes
        -----
        Each detector element is specified in the csv file by a six digit code. The
        first two digits are always either 15 or 11 and correspond to front flush and
        back flush, respectively. Importantly, all the transition segments are back
        flush. The closed and open segments are front flush. Together, the first two
        pairs of numbers specify which segment the detector element is found in. The
        last pair of numbers specifies where the element is within the segment.

        In order to understand what the second pair of numbers signify, it is useful
        to know that the detector is separated azimuthally into 28 segments and radi-
        ally into 6 rings. The first segment is closed, the second is transition, the
        third is open, the fourth is transition, and the pattern repeats. In total,
        there are fourteen transition segments, and seven closed and open segments.
        The transition segments are numbered 1 through 14. Thus, the fourth trans-
        ition segment is specified by the code 1104__, where we have left off the
        last two digits. The closed segments are numbered 1 through 14 odd, and the
        open segments are numbered 1 through 14 even. Thus, the fourth closed and
        open segments are 1507__ and 1508__, respectively.

        The last pair of digits is straightforward to understand. The second-to-last
        digit is the ring number. The last digit is a modifier used to specify the
        element's position within the ring if there is more than one ring element.
        If there is only one element in the ring, the last digit is 0. Ring 5 is the
        only ring with multiple elements per segment. Ring 5 has three elements, and
        they are labelled "left", "center," and "right" from left to right in the
        same motion as the segments are numbered (counter-clockwise). "Left",
        "center", and "right" are numbered 1, 2, and 3, respectively.

        The detector elements are specified more simply in the mapfile by the prefix
        "TQ", standing for "thin quartz", followed by the segment number (two digits),
        an underscore, the letter 'r' and ring number, and sometimes a letter {'l',
        'c', or 'r'} for elements in ring 5.

        Examples:           csv code | map code
                            ---------|---------
                              150110 | TQ01_R1    <-- 1st closed seg, ring 1
                              110851 | TQ16_R5l   <-- 8th trans seg, ring 5, left
                              151120 | TQ21_R2    <-- 11th open seg, ring 2

        """
        with open(filepath, 'r', encoding="utf8") as csvfile:
            reader = csv.DictReader(csvfile, delimiter=',', dialect="unix")
            for row in reader:
                tile: str = row["tile"]
                flush: str = tile[:2]
                num = int(tile[2:4])
                segment: int
                if flush == '11':  # back flush <==> transition segment
                    segment = 2 * num
                else:  # open or closed segment
                    segment = 2 * num - 1
                ring: str = tile[4]
                pos: str = tile[5]
                # The csv rate is GHz, but mapfile has Hz/uA so we convert
                current = 65 # uA; JAPAN uses 100 uA, but values from Xiang used 65 uA
                rate = int(float(row["Average Rate [GHz]"]) * 10**9 / current)
                detector = "TQ" + f"{segment:02}" + "_R" + ring
                match pos:
                    case '1':
                        detector += 'L'
                    case '2':
                        detector += 'C'
                    case '3':
                        detector += 'R'
                try:
                    self.setval(self.rows[detector],
                                self.cols["NormRate(Hz/uA)"], rate)
                except KeyError:
                    print("There was an error constructing the detector name.")
                    print("Import failed.")
                    return

        print("Import successful.")

    def normrate(self, row: str) -> int:
        "Get normrate for specific detector"
        return int(self.entries[self.rows[row]]
                   [self.cols['NormRate(Hz/uA)']])  # Hz/uA

    def save(self):
        """
        Safely save file.

        Marks current file as old, writes new file, and finally removes old file.
        At each step, the function checks the previous step was completed, always
        seeking to preserve data.

        """
        pathname = self.path + self.name
        currentfile = pathname + ".map"
        oldfile = pathname + "_old.map"
        renamed = False
        saved = False
        try:
            if os.path.isfile(oldfile):
                raise RuntimeError(
                    f"Please rename or remove backup file {oldfile} before saving."
                )
            os.rename(currentfile, oldfile)  # mark the current file as backup
            if os.path.isfile(oldfile) and not os.path.isfile(
                currentfile
            ):  # check it has been renamed
                renamed = True
            else:
                raise RuntimeError("Save failed to rename old file.")
            self.writemap(currentfile)  # create new save
            if os.path.isfile(currentfile):  # check that new file was created
                saved = True
            else:
                os.rename(oldfile, currentfile)
                if os.path.isfile(currentfile):
                    raise RuntimeError(
                        "Save failed to create new file. The original file was restored."
                    )
                raise RuntimeError(
                    "Save completely failed. Don't look now, but we might be in deep shit.")

            if renamed and saved:
                os.remove(oldfile)
                if os.path.isfile(oldfile):
                    raise RuntimeWarning(
                        "Save failed to remove the backup file, but a new save was created."
                    )
                print("Save successful.")
            else:
                raise RuntimeWarning(
                    "Everything went wrong, but no errors were raised??"
                )
        except RuntimeError:
            print("Runtime error occurred.")

    def saveas(self, filename: str, path: str = "", ext: str = ".map"):
        """Defaults to same directory as original file unless another directory is specified."""
        saved = False
        if filename[-4:] == ".map":
            if path == "":
                filepath = self.path + filename
            else:
                filepath = path + filename
        else:
            filepath = self.path + filename + ext
        try:
            if os.path.isfile(filepath):
                ans = ""
                while ans not in ('y', 'n'):
                    ans = input(
                        "Are you sure you sure you wish to overwrite existing file? (y/n): "
                    )
                if ans == "y":
                    self.writemap(filepath)
                    saved = True
                else:
                    raise RuntimeError
        except RuntimeError:
            print(f"Call to save {filepath} was cancelled.")
        finally:
            if saved:
                print("File was saved successfully.")
            else:
                print("File was not saved.")

    def setcol(self, col: int, val):
        """Set all values in a column to the provided value."""
        if math.isnan(val):
            raise TypeError("Value must be a number.")
        for row in range(self.countrows()):
            self.setval(row, col, val)

    def setcols(self, cols: list[str], val):
        "Set multiple cols to a specified value."
        if math.isnan(val):
            raise TypeError("Value must be a number.")
        for col in cols:
            self.setcol(self.cols[col], val)

    def setval(self, row: int, col: int, val):
        """Change one value."""
        self.entries[row][col] = val

    def setvals(self, entries, values):  # TODO
        """Change many values by passing a dictionary"""

    def voltperhz(self, row: str) -> float:
        "Get voltperhz value for specific detector"
        return float(self.entries[self.rows[row]][self.cols['VoltPerHz']])

    def writemap(self, fullpath: str):
        "Directly write .map file with ascii encoding"
        with open(fullpath, "w", encoding="ascii") as file:
            file.write(
                f"{'! Name':12}"
                + ' '.join([f"{h:^14}" for h in self.headers])
                + "\n")
            for m in range(self.countrows()):
                file.write(
                    f"{self.names[m]:12}"
                    + " ".join([f"{col:>14}" for col in self.entries[m]])
                    + "\n"
                )
