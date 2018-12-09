using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Diagnostics;
using System.IO;
using System.IO.Pipes;
using System.Linq;
using System.Net;
using System.Security.Cryptography;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;

namespace NZBDrive
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        App()
        {
            InitializeComponent();
        }

        [STAThread]
        static void Main(string[] args)
        {

            // RUN APPLICATION AS SINGLETON:
            bool createdNew = true;
            using (Mutex mutex = new Mutex(true, "NZBDriveApplicationMutex", out createdNew))
            {
                if (createdNew)
                {
                    MainWindow window = new MainWindow(args);

                    // VERSION CHECK:
                    Version curVersion =
                        System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;

                    window.CurrentVersion = curVersion;

                    if (window.LatestVersionCheckDate == null) window.LatestVersionCheckDate = DateTime.MinValue;
                    double timeSinceLastVersionCheck = DateTime.Now.ToOADate() - window.LatestVersionCheckDate.ToOADate();

                    if (timeSinceLastVersionCheck > 7)
                    {
                        Task.Factory.StartNew(() =>
                        {
                            try
                            {
                                WebClient client = new WebClient();
                                string url = "http://www.nzbdrive.com/version?v=" + curVersion.ToString();

                                string newVersionTxt = client.DownloadString(url);
                                
                                window.LatestVersion = new Version(newVersionTxt);
                                window.LatestVersionCheckDate = DateTime.Now;
                            }
                            catch (Exception)
                            { }
                        });
                    }

                    App app = new App();
                    app.Run(window);
                }
                else
                {
                    using (NamedPipeClientStream pipeClient = 
                        new NamedPipeClientStream(".", "NZBDriveApplicationPipe", PipeDirection.Out))
                    {

                        pipeClient.Connect();

                        using (StreamWriter sr = new StreamWriter(pipeClient))
                        {
                            foreach (string arg in args) sr.WriteLine(arg);
                        }
                    }
                }
            }
            Environment.Exit(1);
        }
        static string GetMD5Hash(string input)
        {
            MD5CryptoServiceProvider x = new MD5CryptoServiceProvider();
            byte[] bs = System.Text.Encoding.UTF8.GetBytes(input);
            bs = x.ComputeHash(bs);
            System.Text.StringBuilder s = new System.Text.StringBuilder();
            foreach (byte b in bs)
            {
                s.Append(b.ToString("x2").ToLower());
            }
            string password = s.ToString();
            return password;
        }
    }
}
