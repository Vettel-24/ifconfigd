OBSOLETE /* COPYRIGHT  (C)  1998
OBSOLETE  * THE REGENTS OF THE UNIVERSITY OF MICHIGAN
OBSOLETE  * ALL RIGHTS RESERVED
OBSOLETE  * 
OBSOLETE  * PERMISSION IS GRANTED TO USE, COPY, CREATE DERIVATIVE WORKS 
OBSOLETE  * AND REDISTRIBUTE THIS SOFTWARE AND SUCH DERIVATIVE WORKS 
OBSOLETE  * FOR ANY PURPOSE, SO LONG AS THE NAME OF THE UNIVERSITY OF 
OBSOLETE  * MICHIGAN IS NOT USED IN ANY ADVERTISING OR PUBLICITY 
OBSOLETE  * PERTAINING TO THE USE OR DISTRIBUTION OF THIS SOFTWARE 
OBSOLETE  * WITHOUT SPECIFIC, WRITTEN PRIOR AUTHORIZATION.  IF THE 
OBSOLETE  * ABOVE COPYRIGHT NOTICE OR ANY OTHER IDENTIFICATION OF THE 
OBSOLETE  * UNIVERSITY OF MICHIGAN IS INCLUDED IN ANY COPY OF ANY 
OBSOLETE  * PORTION OF THIS SOFTWARE, THEN THE DISCLAIMER BELOW MUST 
OBSOLETE  * ALSO BE INCLUDED.
OBSOLETE  * 
OBSOLETE  * THIS SOFTWARE IS PROVIDED AS IS, WITHOUT REPRESENTATION 
OBSOLETE  * FROM THE UNIVERSITY OF MICHIGAN AS TO ITS FITNESS FOR ANY 
OBSOLETE  * PURPOSE, AND WITHOUT WARRANTY BY THE UNIVERSITY OF 
OBSOLETE  * MICHIGAN OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING 
OBSOLETE  * WITHOUT LIMITATION THE IMPLIED WARRANTIES OF 
OBSOLETE  * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE 
OBSOLETE  * REGENTS OF THE UNIVERSITY OF MICHIGAN SHALL NOT BE LIABLE 
OBSOLETE  * FOR ANY DAMAGES, INCLUDING SPECIAL, INDIRECT, INCIDENTAL, OR 
OBSOLETE  * CONSEQUENTIAL DAMAGES, WITH RESPECT TO ANY CLAIM ARISING 
OBSOLETE  * OUT OF OR IN CONNECTION WITH THE USE OF THE SOFTWARE, EVEN 
OBSOLETE  * IF IT HAS BEEN OR IS HEREAFTER ADVISED OF THE POSSIBILITY OF 
OBSOLETE  * SUCH DAMAGES.
OBSOLETE  */
OBSOLETE 
OBSOLETE /* $arla: reconnect.h,v 1.5 2002/07/23 15:22:57 lha Exp $ */
OBSOLETE 
OBSOLETE #ifndef _RECONNECT_H
OBSOLETE #define _RECONNECT_H
OBSOLETE 
OBSOLETE void 
OBSOLETE do_replay(char *log_file, int log_entries, VenusFid *changed_fid);
OBSOLETE 
OBSOLETE VenusFid *
OBSOLETE fid_translate(VenusFid *fid_in);
OBSOLETE 
OBSOLETE #endif /* _RECONNECT_H */