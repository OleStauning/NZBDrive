using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public class NZBFilePartCollection: ObservableCollection<NZBFilePart> 
    {
        public NZBFilePartCollection()
            : base()
        { }

        public NZBFilePartCollection(IEnumerable<NZBFilePart> value)
            : base(value)
        { }
    }
}
