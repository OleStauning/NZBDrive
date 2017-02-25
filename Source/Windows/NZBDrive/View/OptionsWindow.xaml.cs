using NZBDrive.Model;
using System;
using System.Collections.Generic;
using System.IO;
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
using System.Windows.Shapes;

namespace NZBDrive.View
{
    public partial class OptionsWindow : Window
    {
        private NZBDriveOptions _options;

//        System.Windows.Forms.Integration.WindowsFormsHost HostCachePath;

        public NZBDriveOptions Options
        {
            get { return _options; }
            set
            {
                if (_options != null) throw new ArgumentException("Options already set");
                _options = value;
                DataContext = _options;
            }
        }

        public OptionsWindow()
        {
            InitializeComponent();
        }

        private void OKButton_Click(object sender, RoutedEventArgs e)
        {
            this.DialogResult = true;
            this.Close();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {

        }

        private void ChooseCacheFolder(object sender, RoutedEventArgs e)
        {
            using (var dialog = new System.Windows.Forms.FolderBrowserDialog())
            {
                dialog.SelectedPath = _options.CachePath;
                System.Windows.Forms.DialogResult result = dialog.ShowDialog();
                if (result == System.Windows.Forms.DialogResult.OK)
                {
                    _options.CachePath = dialog.SelectedPath;
                    (HostCachePath.Child as System.Windows.Forms.TextBox).Text = _options.CachePath;
                }
            }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {

            // Create the MaskedTextBox control.
            var mtbTB = new System.Windows.Forms.TextBox() 
            { 
                AutoCompleteSource= System.Windows.Forms.AutoCompleteSource.FileSystemDirectories,
                AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.Suggest,
                Text = _options.CachePath,
                BorderStyle = System.Windows.Forms.BorderStyle.None,
            };

            mtbTB.TextChanged += mtbTB_TextChanged;

            // Assign the MaskedTextBox control as the host control's child.
            HostCachePath.Child = mtbTB;

            HostCachePath.GotFocus += HostCachePath_GotFocus;

            CachePathLabel.Target = HostCachePath;
        }

        void HostCachePath_GotFocus(object sender, RoutedEventArgs e)
        {
            HostCachePath.Child.Focus();
        }

        void mtbTB_TextChanged(object sender, EventArgs e)
        {
            var textBox = sender as System.Windows.Forms.TextBox;

            if (ValidateExists(textBox, true))
            {
                _options.CachePath = textBox.Text;
            }
        }

        /// <summary>Validates the text content of a textbox towards a path or a file.</summary>
        /// <param name="textBox">The TextBox to examine</param>
        /// <param name="path">True if the TextBox content is a path, otherwise it is a file.</param>
        private bool ValidateExists(System.Windows.Forms.TextBox textBox, bool path)
        {
            if (textBox != null)
            {
                string file = textBox.Text;
                if ((path && Directory.Exists(file)) || (!path && File.Exists(file)))
                {
                    textBox.ForeColor = System.Drawing.Color.FromKnownColor(System.Drawing.KnownColor.WindowText);
                    textBox.BackColor = System.Drawing.Color.FromKnownColor(System.Drawing.KnownColor.Window);
                    return true;
                }
                else
                {
                    textBox.ForeColor = System.Drawing.Color.White;
                    textBox.BackColor = System.Drawing.Color.FromArgb(0xcc, 0, 0);
                    return false;
                }
            }
            return false;
        }
    }
}
