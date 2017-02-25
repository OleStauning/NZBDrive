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
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace NZBDrive.View
{
    /// <summary>
    /// Interaction logic for StatusBox.xaml
    /// </summary>
    public partial class NZBDriveStatusBox : UserControl
    {
        private NZBDriveStatus _status;

        public NZBDriveStatus Status
        {
            get { return _status; }
            set
            {
                if (_status != null) throw new ArgumentException("NZBDriveStatus already set");
                _status = value;
                DataContext = _status;
            }
        }

        public NZBDriveStatusBox()
        {
            InitializeComponent();
        }



    }
}
