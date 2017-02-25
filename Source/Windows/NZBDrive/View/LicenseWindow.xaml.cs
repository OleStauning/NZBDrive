using NZBDrive.Model;
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
using System.Windows.Shapes;

namespace NZBDrive.View
{
    public partial class LicenseWindow : Window
    {
        private NZBDriveModel _model;

        public NZBDriveModel Model
        {
            get { return _model; }
            set
            {
                if (_model != null) throw new ArgumentException("NZBDrive already set");
                _model = value;
                DataContext = _model;
            }
        }

        public LicenseWindow()
        {
            InitializeComponent();
        }

        private void Hyperlink_RequestNavigate(object sender, System.Windows.Navigation.RequestNavigateEventArgs e)
        {
            System.Diagnostics.Process.Start(e.Uri.ToString());
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            Keyboard.Focus(TextBoxLicense);
        }

    }
}
