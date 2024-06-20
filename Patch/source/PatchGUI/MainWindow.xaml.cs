using System;
using System.IO;
using System.Diagnostics;
using System.Windows;
using Microsoft.WindowsAPICodePack.Dialogs;
using System.Text.RegularExpressions;

namespace PatchGUI
{
    /// <summary>
    /// MainWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new CommonOpenFileDialog();
            dlg.Filters.Add(new CommonFileDialogFilter("json", "*.json"));
            dlg.InitialDirectory  = Directory.GetCurrentDirectory();
            dlg.Multiselect = false;
            dlg.InitialDirectory = "";
            var result = dlg.ShowDialog();
            if(result == CommonFileDialogResult.Ok)
            {
                jsonFile.Text = dlg.FileName;
            }
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {
            var dlg = new CommonOpenFileDialog();
            dlg.Filters.Add(new CommonFileDialogFilter("mzt", "*.mzt"));
            dlg.InitialDirectory  = Directory.GetCurrentDirectory();
            dlg.Multiselect = false;
            var result = dlg.ShowDialog();
            if(result == CommonFileDialogResult.Ok)
            {
                mztFile.Text = dlg.FileName;
                var jsonFileName = Path.GetFileName(jsonFile.Text);
                var fileDirectry = Path.GetDirectoryName(mztFile.Text);
                if (jsonFileName == "mz-1z001m.json")
                {
                    var output = fileDirectry + "\\@BOOT-A MZ-2000.bin";
                    outputFile.Text = output;
                }
                else if (jsonFileName == "sb-1520.json")
                {
                    var output = fileDirectry + "\\@BOOT-A MZ-80B.bin";
                    outputFile.Text = output;
                }
                else
                {
                    var output = dlg.FileName;
                    var sdOutput = Regex.Replace(output, ".mzt", "(SD).mzt", RegexOptions.IgnoreCase);
                    outputFile.Text = sdOutput;
                }
            }
        }

        private void Button_Click_2(object sender, RoutedEventArgs e)
        {
            var dlg = new CommonSaveFileDialog();
            dlg.Filters.Add(new CommonFileDialogFilter("mzt", "*.mzt"));
            dlg.Filters.Add(new CommonFileDialogFilter("bin", "*.bin"));
            dlg.InitialDirectory  = Directory.GetCurrentDirectory();
            dlg.DefaultFileName = outputFile.Text;
            var result = dlg.ShowDialog();
            if(result == CommonFileDialogResult.Ok)
            {
                outputFile.Text = dlg.FileName;
            }
        }

        private void Button_Click_3(object sender, RoutedEventArgs e)
        {
            try
            {
                var path = Directory.GetCurrentDirectory();
                Console.WriteLine(path);
                var startInfo = new ProcessStartInfo()
                {
                    FileName = @".\Patch.exe",
                    Arguments = $"\"{jsonFile.Text}\" \"{mztFile.Text}\" \"{outputFile.Text}\"",
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true
                };
                using (var process = Process.Start(startInfo))
                {
                    var output = process.StandardOutput.ReadToEnd();
                    var errorOutput = process.StandardError.ReadToEnd();
                    process.WaitForExit();
                    Message.Text = output + "\n" + errorOutput;
                    Console.WriteLine(output);
                    Console.WriteLine("[Patch.exe] ------");
                    Console.WriteLine(errorOutput);
                }
            }
            catch(Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
        }

        private void Button_Click_4(object sender, RoutedEventArgs e)
        {
            this.Close();
        }
    }
}
