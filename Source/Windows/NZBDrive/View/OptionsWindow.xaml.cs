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

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {

            // Create the MaskedTextBox control.
            var mtbTB = new System.Windows.Forms.TextBox() 
            { 
                AutoCompleteSource= System.Windows.Forms.AutoCompleteSource.FileSystemDirectories,
                AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.Suggest,
                BorderStyle = System.Windows.Forms.BorderStyle.None,
            };

        }


    }
}
