using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO.Ports;
using System.IO;
using System.Timers;
using Microsoft.Win32;
using System.Threading;
using System.ComponentModel;
using System.Xml;
using System.Text.RegularExpressions;
using System.Diagnostics;

namespace MCM6000_UI
{

    public partial class MainWindow
    {
        public static string log_dir = "log";
        string log_file_path = log_dir + "\\log.txt";
        string stepper_log_folder_path = "\\" + log_dir + "\\stepper log";

        XmlDocument xmlDoc = new XmlDocument();

        string log_buffer;
        int log_buff_num_lines = 0;

        private void create_log_file(string filename)
        {
            // delete older files, only keep the newest 20
            var files = new DirectoryInfo(log_dir).EnumerateFiles()
                .OrderByDescending(f => f.CreationTime)
                .Skip(25)
                .ToList();
                files.ForEach(f => f.Delete());

            File.CreateText(log_file_path).Close();
            using (StreamWriter w = File.AppendText(log_file_path))
            {
                string new_line = "File  created on ";
                new_line += "[" + $"{DateTime.Now.ToLongTimeString()} {DateTime.Now.ToLongDateString()}" + "]";
                new_line += "\n-------------------------------------------------------------------------------------------";
                new_line += "-------------------------------------------------------------------\n";
                w.WriteLine($"{new_line}");
            }
        }

        public void load_log_file()
        {
            if (!Directory.Exists(log_dir))
                Directory.CreateDirectory(log_dir);

            string filename = System.IO.Path.GetFullPath(log_file_path);

            if (!(File.Exists(log_file_path)))
            {
                create_log_file(log_file_path);
            }

            string allLogText = File.ReadAllText(filename);

            //  rtb_log.Dispatcher.Invoke(() =>
            // {
            rtb_log.Document.Blocks.Add(new Paragraph(new Run(allLogText)));
            // });
        }

        public void load_log_xml_file()
        {
            // Get the file's location.
            string filename = System.AppDomain.CurrentDomain.BaseDirectory;
            filename = System.IO.Path.GetFullPath(filename) + "log\\log.xml";

            string allText = File.ReadAllText(filename);
            log_buffer = "";

            try
            {
                xmlDoc.LoadXml(allText);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error: " + ex.Message);
            }
        }

        public void log()
        {
            byte slot_log = (byte)(header[5] - 0x21 + 1); // Starts on 0 so add 1
            byte log_type = ext_data[0];
            byte log_id = ext_data[1];
            int var1 = ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24;
            int var2 = ext_data[6] | ext_data[7] << 8 | ext_data[8] << 16 | ext_data[9] << 24;
            int var3 = ext_data[10] | ext_data[11] << 8 | ext_data[12] << 16 | ext_data[13] << 24;
            string class_s = "No Class";
            string id_type_s = "No Id type";
            string id_name_s = "No Id name";
            string source_s = "No Source";
            string description_s = "No Description";
            string var1_s = "";
            string var2_s = "";
            string var3_s = "";

            XmlNodeList xnList = xmlDoc.SelectNodes("/Logs/Log_type");
            foreach (XmlNode xn in xnList)
            {
                if (Convert.ToString(log_type) == xn["Id"].InnerText)
                {
                    id_type_s = xn["Id_type"].InnerText;
                    description_s = xn["Description"].InnerText;
                }
            }
            string new_line = "";

            new_line += "[" + $"{DateTime.Now.ToLongTimeString()} {DateTime.Now.ToLongDateString()}" + "]";
            new_line += "  [" + description_s + "]  ";

            XmlNodeList idList = xmlDoc.SelectNodes("/Logs/" + id_type_s);
            foreach (XmlNode xn in idList)
            {
                if (Convert.ToString(log_id) == xn["Id"].InnerText)
                {
                    id_name_s = xn["Id_Name"].InnerText;
                    source_s = xn["Source"].InnerText;
                    description_s = xn["Description"].InnerText;
                    class_s = xn["Class"].InnerText;
                    var1_s = xn["Var1"].InnerText;
                    var2_s = xn["Var2"].InnerText;
                    var3_s = xn["Var3"].InnerText;
                }
            }

            new_line += source_s + " ";
            if (slot_log != 18) // if not the mother_board_id
                new_line += Convert.ToString(slot_log);
            new_line += "   " + class_s;

            // encoder counts is held in var 1 and we need to convert encoder count to um
            if (var1_s == "um:") // for stepper
            {
                double enc = var1 * slot_nm_per_count[slot_log - 1] / 1e3; ;
                if (var1_s != "")
                    new_line += var1_s + " " + Convert.ToString(enc) + " ";
            }
            else if (var1_s != "") // print whatever is in var1
            {
                new_line += var1_s + " " + Convert.ToString(var1) + " ";
            }

            new_line += " [" + id_name_s + "]";
            new_line += " ->" + description_s + " ";
            // new_line += "\n\r-------------------------------------------------------------------------------------------"; 
            //  new_line += "--------------------------------------------------------------------------\n\r";

            new_line += Environment.NewLine;
            log_buffer += new_line;
            log_buff_num_lines += 1;

            if (log_buff_num_lines >= 100)
            {
                flush_log_buffer();
            }
        }

        private void flush_log_buffer()
        {
            if (!Directory.Exists(log_dir))
            { Directory.CreateDirectory(log_dir); }
            using (StreamWriter w = File.AppendText(log_file_path))
            {
                w.Write($"{log_buffer}");
                log_buffer = "";
            }
            log_buff_num_lines = 0;

            // check if file is getting to large.  If so rename with the curretn date and create a new one.
            var fileInfo = new FileInfo(log_file_path);
            long file_size = fileInfo.Length / 1024;  // size in KB
            if (file_size > 5000) // 5000kb or 5 MB
            {
                Thread.Sleep(20);

                // rename this file with the date/time extension and create a new log file
                string log_file_backup_path = log_dir + "\\log_";
                log_file_backup_path += string.Format("text-{0:yyyy-MM-dd_hh-mm-ss-tt}.txt", DateTime.Now);

                if (File.Exists(log_file_path))
                {
                    File.Copy(log_file_path, log_file_backup_path, true);
                    Thread.Sleep(20);
                    File.Delete(log_file_path);
                    Thread.Sleep(20);

                }
                if (!(File.Exists(log_file_path)))
                {
                    create_log_file(log_file_path);
                }
            }
        }


        private void btRefresh_log_file_Click(object sender, RoutedEventArgs e)
        {
            rtb_log.Document.Blocks.Clear();
            flush_log_buffer();
            load_log_file();
            rtb_log.Focus();
            rtb_log.ScrollToEnd();
        }

        /// <summary>
        /// Takes a string input and searches richtextbox in the direction specified.  
        /// </summary>
        private void SelectWord(string input, LogicalDirection direction)
        {
            RichTextBox rtb = rtb_log;

            // get the richtextbox text
            string richText = new TextRange(rtb.Document.ContentStart, rtb.Document.ContentEnd).Text;

            if (string.IsNullOrWhiteSpace(richText) || string.IsNullOrWhiteSpace(input))
            {
                //  lbl_Status.Content = "Please provide search text or source text to search from";
                return;
            }

            TextRange textRange = new TextRange(rtb_log.Document.ContentStart, rtb_log.Document.ContentEnd);

            //get the richtextbox text
            string textBoxText = textRange.Text;

            Regex regex = new Regex(input, RegexOptions.IgnoreCase | RegexOptions.Compiled);
            int count_MatchFound = Regex.Matches(textBoxText, regex.ToString(), options: RegexOptions.IgnoreCase).Count;
            lb_log_results.Content = "Found : " + count_MatchFound;

            TextPointer currentStartposition = rtb.Selection.Start;
            TextPointer currentEndposition = rtb.Selection.End;
            TextPointer position;
            TextPointer previousPosition;
            string textLine = null;
            if (direction == LogicalDirection.Forward)
            {
                position = currentStartposition.GetLineStartPosition(1);
                previousPosition = currentEndposition;
                if (position != null)
                    textLine = new TextRange(previousPosition, position).Text;
            }
            else
            {
                position = currentStartposition.GetLineStartPosition(0);
                previousPosition = currentStartposition;
                if (position != null)
                    textLine = new TextRange(position, previousPosition).Text;
            }

            while (position != null)
            {
                int indexInRun;
                if (direction == LogicalDirection.Forward)
                    indexInRun = textLine.IndexOf(input, StringComparison.CurrentCultureIgnoreCase);
                else
                    indexInRun = textLine.LastIndexOf(input, StringComparison.CurrentCultureIgnoreCase);

                if (indexInRun >= 0)
                {
                    TextPointer nextPointer = null;
                    if (direction == LogicalDirection.Forward)
                        position = previousPosition;

                    int inputLength = input.Length;
                    while (nextPointer == null)
                    {
                        if (position.GetPointerContext(LogicalDirection.Forward) == TextPointerContext.Text && nextPointer == null) //checks to see if textpointer is actually text
                        {
                            string textRun = position.GetTextInRun(LogicalDirection.Forward);
                            if (textRun.Length - 1 < indexInRun)
                                indexInRun -= textRun.Length;
                            else //found the start position of text pointer
                            {
                                position = position.GetPositionAtOffset(indexInRun);
                                nextPointer = position;
                                while (inputLength > 0)
                                {
                                    textRun = nextPointer.GetTextInRun(LogicalDirection.Forward);
                                    if (textRun.Length - indexInRun < inputLength)
                                    {
                                        inputLength -= textRun.Length;
                                        indexInRun = 0; //after the first pass, index in run is no longer relevant
                                    }
                                    else
                                    {
                                        nextPointer = nextPointer.GetPositionAtOffset(inputLength);
                                        rtb.Selection.Select(position, nextPointer);
                                        rtb.Focus();

                                        //moves the scrollbar to the selected text
                                        Rect r = position.GetCharacterRect(LogicalDirection.Forward);
                                        double totaloffset = r.Top + rtb.VerticalOffset;
                                        rtb.ScrollToVerticalOffset(totaloffset - rtb.ActualHeight / 2);
                                        return; //word is selected and scrolled to. Exit method
                                    }
                                    nextPointer = nextPointer.GetNextContextPosition(LogicalDirection.Forward);
                                }


                            }
                        }
                        position = position.GetNextContextPosition(LogicalDirection.Forward);
                    }
                }

                previousPosition = position;
                if (direction == LogicalDirection.Forward)
                {
                    position = position.GetLineStartPosition(1);
                    if (position != null)
                        textLine = new TextRange(previousPosition, position).Text;
                }
                else
                {
                    position = position.GetLineStartPosition(-1);
                    if (position != null)
                        textLine = new TextRange(position, previousPosition).Text;
                }

            }

            //if next/previous word is not found, leave the current selected word selected
            rtb.Selection.Select(currentStartposition, currentEndposition);
            rtb.Focus();
        }

        private void btSearch_up_log_file_Click(object sender, RoutedEventArgs e)
        {
            SelectWord(tb_search_log.Text, LogicalDirection.Backward);

        }
        private void btSearch_down_log_file_Click(object sender, RoutedEventArgs e)
        {
            SelectWord(tb_search_log.Text, LogicalDirection.Forward);

        }

        private void btOpen_log_file_Click(object sender, RoutedEventArgs e)
        {
            string log_file = string.Empty;

            // open file dialog
            Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();
            dlg.Title = "Select .txt log file using this Dialog Box";
            dlg.DefaultExt = ".txt";
            dlg.Filter = "txt Files|*.txt";
            Nullable<bool> result = dlg.ShowDialog();
            log_file = dlg.FileName;

            string allLogText = File.ReadAllText(log_file);

            rtb_log.Document.Blocks.Clear();
            rtb_log.Document.Blocks.Add(new Paragraph(new Run(allLogText)));
            rtb_log.ScrollToEnd();
        }

        private void bnt_Search_All_Click(object sender, RoutedEventArgs e)
        {
            TextRange textRange = new TextRange(rtb_log.Document.ContentStart, rtb_log.Document.ContentEnd);

            //clear up highlighted text before starting a new search
            textRange.ClearAllProperties();
            lb_log_results.Content = "";

            //get the richtextbox text
            string textBoxText = textRange.Text;

            //get search text
            string searchText = tb_search_log.Text;

            if (string.IsNullOrWhiteSpace(textBoxText) || string.IsNullOrWhiteSpace(searchText))
            {
                lb_log_results.Content = "Empty";
            }

            else
            {
                Regex regex = new Regex(searchText, RegexOptions.IgnoreCase | RegexOptions.Compiled);
                int count_MatchFound = Regex.Matches(textBoxText, regex.ToString(), options: RegexOptions.IgnoreCase).Count;

                for (TextPointer startPointer = rtb_log.Document.ContentStart;
                            startPointer.CompareTo(rtb_log.Document.ContentEnd) <= 0;
                                startPointer = startPointer.GetNextContextPosition(LogicalDirection.Forward))
                {
                    //check if end of text
                    if (startPointer.CompareTo(rtb_log.Document.ContentEnd) == 0)
                    {
                        break;
                    }

                    //get the adjacent string
                    string parsedString = startPointer.GetTextInRun(LogicalDirection.Forward);

                    //check if the search string present here
                    int indexOfParseString = parsedString.IndexOf(searchText, StringComparison.CurrentCultureIgnoreCase);

                    if (indexOfParseString >= 0) //present
                    {
                        //setting up the pointer here at this matched index
                        startPointer = startPointer.GetPositionAtOffset(indexOfParseString);

                        if (startPointer != null)
                        {
                            //next pointer will be the length of the search string
                            TextPointer nextPointer = startPointer.GetPositionAtOffset(searchText.Length);

                            //create the text range
                            TextRange searchedTextRange = new TextRange(startPointer, nextPointer);

                            //color up 
                            searchedTextRange.ApplyPropertyValue(TextElement.BackgroundProperty, new SolidColorBrush(Colors.Red));
                            searchedTextRange.ApplyPropertyValue(TextElement.ForegroundProperty, new SolidColorBrush(Colors.White));

                            //add other setting property

                        }
                    }
                }

                //update the label text with count

                lb_log_results.Content = "Found : " + count_MatchFound;

            }
        }


    }
}