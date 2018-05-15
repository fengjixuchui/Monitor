///////////////////////////////////////////////////////////////////////////////
///

/// Author(s)        : icedxu
///
/// Purpose          : 文件监控
///
/// Revisions:
///  0000 [2017-05-02] Initial revision.
///
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <string.h>
#ifdef __cplusplus
}; // extern "C"
#endif

#include "FileEncrypt.h"
#include "tools.h"

#define PAGEDCODE   code_seg("PAGE")  
#define LOCKEDCODE  code_seg() 
 

/************************************************************************/
/* 全局变量                                                              */
/************************************************************************/
//
//  This is a lookAside list used to allocate our pre-2-post structure.
//
//NPAGED_LOOKASIDE_LIST Pre2PostContextList;

//进程名的偏移
ULONG  ProcessNameOffset = 0;

//minifilter 句柄
//PFLT_FILTER gFilterHandle;

//客户端句柄，以后有用
PFLT_PORT gClientPort;

//进程链表头
PTYPE_KEY_PROCESS key_word_header;

//全局开关
BOOLEAN IS_SYSTEM_OPEN =FALSE;



//通信端口句柄
PFLT_PORT serverPort=NULL;



#ifdef __cplusplus
extern "C" {
#endif

	////驱动入口
	NTSTATUS DriverEntry(
		IN OUT PDRIVER_OBJECT   DriverObject,
		IN PUNICODE_STRING      RegistryPath
		)
	{
		NTSTATUS status;
		PSECURITY_DESCRIPTOR  sd;
		OBJECT_ATTRIBUTES oa;
		UNICODE_STRING portName=RTL_CONSTANT_STRING(SERVER_PORT_NAME);
			
		KdPrint(("DriverEntry \n"));

		//过滤掉系统自带的一些进程
	//	CHAR StrategyString[]="System;svchost.exe;explorer.exe;vmtoolsd.exe;";
	//key_word_header = GetStrategyFromString(StrategyString);
	
		
	

		//获取进程名称偏移
		ProcessNameOffset=GetProcessNameOffset();

		PsSetCreateProcessNotifyRoutine(MyMiniFilterProcessNotify, FALSE);
		PsSetLoadImageNotifyRoutine(MyMiniFilterLoadImage);//

		InitializeListHead(&HidePathListHeader); //初始化链表头
	//	KeInitializeSpinLock(&HidePathListLock); //初始化链表锁

	


	


		//初始化Lookaside对象,不分页
	/*	ExInitializeNPagedLookasideList( &Pre2PostContextList,
			NULL,
			NULL,
			0,
			sizeof(PRE_2_POST_CONTEXT),
			PRE_2_POST_TAG,
			0 );*/

		//注册
		status=FltRegisterFilter(DriverObject,
								&FilterRegistration,
								&gFilterHandle);
			
		ASSERT(NT_SUCCESS(status));		
		if (NT_SUCCESS(status))
		{
			//启动过滤器
			status=FltStartFiltering(gFilterHandle);
			if(!NT_SUCCESS(status))
			{
			//	ExDeleteNPagedLookasideList( &Pre2PostContextList );
				FltUnregisterFilter(gFilterHandle);
			}

			//以下与通信相关
			status = FltBuildDefaultSecurityDescriptor(&sd,FLT_PORT_ALL_ACCESS);
			if (!NT_SUCCESS(status))
			{
				////////////DbgPrint("通信端口中申请默认安全级失败\n");
				return status;
			}
			InitializeObjectAttributes(&oa,
										&portName,
										OBJ_KERNEL_HANDLE|OBJ_CASE_INSENSITIVE,
										NULL,
										sd);
				//注册通信端口
			status = FltCreateCommunicationPort(gFilterHandle,
												&serverPort,
												&oa,
												NULL,
												MyConnectionCallback,
												MyDisconnectCallback,
												MyMessageCallback,
												SERVER_MAX_COUNT
												);

			if (!NT_SUCCESS(status))
			{
				////////////DbgPrint("注册服务器端口失败 \n");
				//ExDeleteNPagedLookasideList( &Pre2PostContextList );
				FltUnregisterFilter(gFilterHandle);
			}
				FltFreeSecurityDescriptor( sd );
		} 
		//	StartThread();

		return status;
	}

#ifdef __cplusplus
}; // extern "C"
#endif




///////////////////////////////Create/////////////////////////////////////


#pragma  LOCKEDCODE
FLT_POSTOP_CALLBACK_STATUS
CreatePost(
			 __inout PFLT_CALLBACK_DATA Data,
			 __in PCFLT_RELATED_OBJECTS FltObjects,
			 __in PVOID CompletionContext,
			 __in FLT_POST_OPERATION_FLAGS Flags
			 )
{
	FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
	
	NTSTATUS status;

	
	//检查中断级
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		KdPrint(("CreatePost IRQL\n"));
		return retValue;
	}
		//下句用来判断此文件操作是创建、打开等
		//UCHAR create_options = (UCHAR)((Data->Iopb->Parameters.Create.Options>>24)&0xff);
		//if (create_options == FILE_CREATE)
		//{ 	
		//	/*新建文件*/
		//	STREAM_HANDLE_CONTEXT temCtx;	
		//		status = GetFileInformation(Data,FltObjects,&temCtx);

		//		if (NT_SUCCESS(status))
		//		{
		//			PCHAR procName=GetCurrentProcessName(ProcessNameOffset);
		//			PEPROCESS  p = FltGetRequestorProcess(Data);
		//			ULONG ProcessId = FltGetRequestorProcessId(Data);  
		//			ULONG ThreadId = (ULONG)PsGetThreadId(Data->Thread); 
		//			EnumProcess(ThreadId);

		//			//获取系统运行时间，此函数返回值已被处理只返回开机到到现在的秒数，可以放在日志的开头
		//	
		//			//	KdPrint((" ThreadId = %u \n",ThreadId));
		//			ULONG Time = GetTime();
		//			CHAR T[100]={0},PID[100]={0},PPID[100] ={0}  ;

		//			IntegerToChar(Time,T);
		//			IntegerToChar(ProcessId,PPID);
		//			IntegerToChar(ThreadId,PID);
		//			//KdPrint(("%s \n",T));
		//			//KdPrint(("PPID = %s,PID = %s \n",PPID,PID));

		//			CHAR FileName[260] ={0}; 
  //                //长度木有起作用
		//			NPUnicodeStringToChar(&temCtx.fileVolumeName, FileName,temCtx.fileVolumeName.Length);
		//			//KdPrint(("yuan = %wZ ; FileName = %s,Length = %d",&temCtx.fileVolumeName,FileName,temCtx.fileVolumeName.Length));
		//			//T=0;OP=1;C=test.exe;  PID=123;PPID=321;P=\\Device\\HarddiskVolume1\\test;S=15
		//			//CHAR STR[260] = {"T="};
		//			CHAR STR[260] = "XXX;";


		//			if (!IsSecretProcess(procName))
		//			{
		//				/*strcat(STR,T);strcat(STR,";OP=1;C="); 
		//				strcat(STR,procName);strcat(STR,";PID=");
		//				strcat(STR,PID);strcat(STR,";PPID="); strcat(STR,PPID);strcat(STR,";P=");
		//				strcat(STR,FileName);strcat(STR,";S="); strcat(STR,"1555\r\n");
		//				KdPrint(("%s",STR));*/

		//				strcat(STR,T);strcat(STR,";1;"); 
		//				strcat(STR,procName);strcat(STR,";");
		//				strcat(STR,PID);strcat(STR,";"); strcat(STR,PPID);strcat(STR,";");
		//				strcat(STR,FileName);strcat(STR,";"); strcat(STR,"1555\r\n");
		//				//KdPrint(("%s",STR));

		//				PLOG_LIST pathListNode ,pathList;
		//				pathListNode = (PLOG_LIST)ExAllocatePool(NonPagedPool,sizeof(LOG_LIST));
		//				if (pathListNode == NULL)
		//				{
		//					KdPrint(("队列申请失败  \n"));  
		//				}
		//				//wcscpy(pathListNode->xxPath,pszDest);
		//				RtlCopyMemory(pathListNode->xxPath,STR,strlen(STR));
		//				//InsertTailList(&HidePathListHeader,&pathListNode->listNode);//插入队尾

		//			}
		//			/*KdPrint(("文件类型 = %d,文件路径:= %d ", temCtx.fileStyle.Length,temCtx.fileFullPath.Length));
		//			KdPrint(("所在卷:= %d 父目录 = %d\n", temCtx.fileVolumeName.Length,temCtx.fileName.Length));
		//			KdPrint(("文件路径:= %wZ , 文件类型 = %wZ", &temCtx.fileFullPath,&temCtx.fileStyle));	
		//			KdPrint(("所在卷 =%wZ,父目录=%wZ \n ",&temCtx.fileVolumeName,&temCtx.fileName));	*/
		//		} 	
		//}






	


		PSTREAM_HEAD headCtx;
	
		status=FltGetStreamHandleContext(FltObjects->Instance,FltObjects->FileObject,(PFLT_CONTEXT *)&headCtx);
		if(NT_SUCCESS(status))
		{
			if (NULL!= headCtx)
			{
				KdPrint(("4\n"));
				FltReleaseContext(headCtx);
			}
			KdPrint(("44\n"));
			return FLT_POSTOP_FINISHED_PROCESSING;
		} 

		if (NULL!= headCtx)
		{
			KdPrint(("3\n"));
			FltReleaseContext(headCtx);
		}
		
		STREAM_HEAD headStream ;
		headStream.isRead = FALSE;
		LARGE_INTEGER offset;
		offset.QuadPart = 0;
	    ULONG readLen = 0;
		status = STATUS_SUCCESS;
	   status = FltReadFile(FltObjects->Instance,
		                	FltObjects->FileObject,
							&offset,
							LENGTH_READ,
							(PVOID)headStream.fileHead,
							FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
							&readLen,
							NULL,
							NULL);
	
		if (NT_SUCCESS(status))
		{
			//KdPrint(("head is %s ，读取的长度是  %ld ,%ld\n",headStream.fileHead,readLen,strlen(headStream.fileHead)));
			CHAR tmp[LENGTH_READ] = {0};
			CharToHex(headStream.fileHead,tmp);
			//KdPrint(("char fileHead = %s\n",tmp));


			PSTREAM_HEAD  CTX;
			//KdPrint(("CreatePost\n"));
			//创建流上下文，分配一个上下文结构体
              status =FltAllocateContext( FltObjects->Filter,
										FLT_STREAMHANDLE_CONTEXT,
										sizeof(STREAM_HEAD), //上下文大小
										NonPagedPool, //池子类型
										(PFLT_CONTEXT *)&CTX); //返回的上下文
				if (!NT_SUCCESS(status)) 
				{
					KdPrint(("FltAllocateContext分配失败， 错误代码status=%08x \n",status));
					return retValue;
				}

				PFLT_CONTEXT oldCtx = NULL;	
				//设置文件流句柄上下文
				status=FltSetStreamHandleContext(FltObjects->Instance, 
					FltObjects->FileObject,
					FLT_SET_CONTEXT_REPLACE_IF_EXISTS,  //操作类型
					CTX, //新的上下文
					&oldCtx);  //以前存在的旧的上下文		
				if (!NT_SUCCESS(status))
				{
					//KdPrint(("FltSetStreamHandleContext设置失败,错误代码status=%08x \n",status));
					if (NULL!= CTX)
					{
						FltReleaseContext(CTX);
					}
					return retValue;
				}	
				//KdPrint(("FltSetStreamHandleContext成功 \n"));
				CTX->isRead = TRUE;
				RtlStringCbCopyA(CTX->fileHead,strlen(tmp),tmp);

				//KdPrint(("fileHead = %s\n",CTX->fileHead));
				if (NULL!= CTX)
				{
				//	KdPrint(("FltSetStreamHandleContext  释放 \n"));
					FltReleaseContext(CTX);
				}

		}
		
	return retValue;
}


#pragma  LOCKEDCODE
FLT_PREOP_CALLBACK_STATUS
	WritePre(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID *CompletionContext
	)
{
	PPRE_2_POST_CONTEXT p2pCtx = (PPRE_2_POST_CONTEXT)CompletionContext;
	UNREFERENCED_PARAMETER( FltObjects );

	NTSTATUS status;
	STREAM_HANDLE_CONTEXT temCtx;	
	//检查中断级
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	status = GetFileInformation(Data,FltObjects,&temCtx);
	
if (NT_SUCCESS(status))
		{
			//KdPrint(("path = %wZ",&temCtx.fileFullPath));
			
			
			PCHAR procName=GetCurrentProcessName(ProcessNameOffset);
			PEPROCESS  p = FltGetRequestorProcess(Data);
			ULONG ProcessId = FltGetRequestorProcessId(Data);  
			ULONG ThreadId = (ULONG)PsGetThreadId(Data->Thread);  
			
			EnumProcess(ThreadId);
		//	KdPrint((" ThreadId = %u \n",ThreadId));
			ULONG Time = GetTime();
			CHAR T[100]={0},PID[100]={0},PPID[100] ={0}  ;

			IntegerToChar(Time,T);
		    IntegerToChar(ProcessId,PPID);
		    IntegerToChar(ThreadId,PID);
			//KdPrint(("%s \n",T));
		   // KdPrint(("PPID = %s,PID = %s \n",PPID,PID));

	    	CHAR FileName[260] ={0}; 

			NPUnicodeStringToChar(&temCtx.fileVolumeName, FileName,temCtx.fileVolumeName.Length);
			//KdPrint((" FileName = %s",FileName));
			//T=0;OP=1;C=test.exe;  PID=123;PPID=321;P=\\Device\\HarddiskVolume1\\test;S=15
			//CHAR STR[260] = {"T="};
			CHAR STR[260] = "XXX;";

















































			if (!IsSecretProcess(procName))
			{





				PSTREAM_HEAD headCtx;

				status=FltGetStreamHandleContext(FltObjects->Instance,FltObjects->FileObject,(PFLT_CONTEXT *)&headCtx);
				if(!NT_SUCCESS(status))
				{
					//KdPrint(("write 获取上下文失败  status = 08%x\n",status));
					if (NULL!= headCtx)
					{
						FltReleaseContext(headCtx);
					}

					return FLT_PREOP_SUCCESS_NO_CALLBACK;
				} 

				if (NULL!= headCtx)
				{
					FltReleaseContext(headCtx);
				}

				STREAM_HEAD headStream ;
				headStream.isRead = FALSE;
				LARGE_INTEGER offset;
				offset.QuadPart = 0;
				ULONG readLen = 0;

				status = FltReadFile(FltObjects->Instance,
				FltObjects->FileObject,
				&offset,
				LENGTH_READ,
				(PVOID)headStream.fileHead,
				FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
				&readLen,
				NULL,
				NULL);

				//RtlStringCbCopyA(headStream.fileHead,40,"0");

				if (NT_SUCCESS(status))
				{
					//KdPrint(("head is %s ，读取的长度是  %ld ,%ld\n",headStream.fileHead,readLen,strlen(headStream.fileHead)));
					CHAR tmp[LENGTH_READ] = {0};
					CharToHex(headStream.fileHead,tmp);
					KdPrint(("原来的： %s,现在的：%s",headCtx->fileHead,tmp));

				}



























				/*strcat(STR,T);strcat(STR,";OP=4;C="); strcat(STR,procName);strcat(STR,";PID=");
				strcat(STR,PID);strcat(STR,";PPID="); strcat(STR,PPID);strcat(STR,";P=");
				strcat(STR,FileName);strcat(STR,";S="); strcat(STR,"1555\r\n");
				KdPrint(("%s",STR));*/


				strcat(STR,T);strcat(STR,";4;"); 
				strcat(STR,procName);strcat(STR,";");
				strcat(STR,PID);strcat(STR,";"); strcat(STR,PPID);strcat(STR,";");
				strcat(STR,FileName);strcat(STR,";"); strcat(STR,"1555\n");
































				PLOG_LIST pathListNode ,pathList;
				pathListNode = (PLOG_LIST)ExAllocatePool(NonPagedPool,sizeof(LOG_LIST));
				if (pathListNode == NULL)
				{
					KdPrint(("队列申请失败  \n"));  
				}
				//wcscpy(pathListNode->xxPath,pszDest);
				RtlCopyMemory(pathListNode->xxPath,STR,strlen(STR));
				InsertTailList(&HidePathListHeader,&pathListNode->listNode);//插入队尾



			}
			
					//KdPrint(("文件类型 = %d,文件路径:= %d ", temCtx.fileStyle.Length,temCtx.fileFullPath.Length));
					//KdPrint(("所在卷:= %d 父目录 = %d\n", temCtx.fileVolumeName.Length,temCtx.fileName.Length));
					//KdPrint(("文件路径:= %wZ , 文件类型 = %wZ", &temCtx.fileFullPath,&temCtx.fileStyle));	
					//KdPrint(("所在卷 =%wZ,父目录=%wZ \n ",&temCtx.fileVolumeName,&temCtx.fileName));	

		
	//		ULONG replyLength;  
	//		SCANNER_REPLY   Reply = {0};  
	//		replyLength = sizeof(SCANNER_REPLY);  

	//		PSCANNER_NOTIFICATION notification =(PSCANNER_NOTIFICATION) ExAllocatePool(NonPagedPool,sizeof(SCANNER_NOTIFICATION)); 
	////		if (notification == NULL)return ;  
	//		RtlZeroMemory(notification, sizeof(SCANNER_NOTIFICATION));  
	//		//notification->bCreate = Create;  
	//		RtlCopyMemory(notification->ProcessName, pName,strlen(pName)+1);

	//		status = FltSendMessage(gFilterHandle,   //句柄
	//			                    &gClientPort, //客户端端口
	//			                    notification,//发送缓冲
	//			                    sizeof(SCANNER_NOTIFICATION), //发送缓冲区的大小
	//								&Reply,
	//								&replyLength,
	//								NULL
	//								);

	//		if (NT_SUCCESS(status))  
	//			{  
	//			  KdPrint(("发送成功  %d\n", replyLength));  
	//			}  
	//		else  
	//			{  
	//				KdPrint(("发送失败  status = %08x\n",status));  
	//			}  



		   /* CHAR pszDest[30];
		    ULONG cbDest = 30;
			LPCSTR pszFormat = "%s %d + %d = %d.";
			CHAR* pszTxt = "The answer is";

			RtlStringCbPrintfA(pszDest, cbDest, pszFormat, pszTxt, 1, 2, "3\n");*/
		//	KdPrint(("%s",pszDest));




			



			//设置事件为有信号，通知  
			//KeSetEvent(g_pEventObject, 0, FALSE);  


			//if (pathListNode == NULL)
			//{
			//	return   FLT_POSTOP_FINISHED_PROCESSING;;
			//}
			//wcscpy(pathListNode->xxPath,L"你好！");
			//KeAcquireSpinLock(&HidePathListLock,&Irql);
			//InsertTailList(&HidePathListHeader,&pathListNode->listNode);
			//KeReleaseSpinLock(&HidePathListLock,Irql);

		} 




		//PSTREAM_HEAD headCtx;
		//status=FltGetStreamHandleContext(FltObjects->Instance,FltObjects->FileObject,(PFLT_CONTEXT *)&headCtx);
		//if(!NT_SUCCESS(status))
		//{
		//	KdPrint(("write 获取上下文流句柄失败 \n"));
		//	return FLT_POSTOP_FINISHED_PROCESSING;
		//} 


		//LARGE_INTEGER offset;
		//STREAM_HEAD headStream;
		//offset.QuadPart = 0;
		//ULONG readLen = 0;

		//status = FltReadFile(FltObjects->Instance,
		//	FltObjects->FileObject,
		//	&offset,
		//	LENGTH_READ,
		//	(PVOID)headStream.fileHead,
		//	FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET|FLTFL_IO_OPERATION_NON_CACHED,
		//	&readLen,
		//	NULL,
		//	NULL);
		//if (NT_SUCCESS(status))
		//{
		//	if (strncmp(headStream.fileHead,headCtx->fileHead,LENGTH_READ))
		//	{
		//		KdPrint(("相同\n"));
		//	}
		//}


		//if (NULL!=headCtx) //icedxu
		//{
		//	FltReleaseContext(headCtx);
		//}


	return FLT_PREOP_SUCCESS_NO_CALLBACK;
   
}







#pragma  LOCKEDCODE
FLT_POSTOP_CALLBACK_STATUS
	WritePost(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
	)
{
	PPRE_2_POST_CONTEXT p2pCtx = (PPRE_2_POST_CONTEXT)CompletionContext;
	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( Flags );
	NTSTATUS status;
	STREAM_HANDLE_CONTEXT temCtx;	
	//检查中断级
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	status = GetFileInformation(Data,FltObjects,&temCtx);
	
if (NT_SUCCESS(status))
		{
			//KdPrint(("path = %wZ",&temCtx.fileFullPath));
			
			
			PCHAR procName=GetCurrentProcessName(ProcessNameOffset);
			PEPROCESS  p = FltGetRequestorProcess(Data);
			ULONG ProcessId = FltGetRequestorProcessId(Data);  
			ULONG ThreadId = (ULONG)PsGetThreadId(Data->Thread);  
			
			EnumProcess(ThreadId);
		//	KdPrint((" ThreadId = %u \n",ThreadId));
			ULONG Time = GetTime();
			CHAR T[100]={0},PID[100]={0},PPID[100] ={0}  ;

			IntegerToChar(Time,T);
		    IntegerToChar(ProcessId,PPID);
		    IntegerToChar(ThreadId,PID);
			//KdPrint(("%s \n",T));
		   // KdPrint(("PPID = %s,PID = %s \n",PPID,PID));

	    	CHAR FileName[260] ={0}; 

			NPUnicodeStringToChar(&temCtx.fileVolumeName, FileName,temCtx.fileVolumeName.Length);
			//KdPrint((" FileName = %s",FileName));
			//T=0;OP=1;C=test.exe;  PID=123;PPID=321;P=\\Device\\HarddiskVolume1\\test;S=15
			//CHAR STR[260] = {"T="};
			CHAR STR[260] = "XXX;";

















































			if (!IsSecretProcess(procName))
			{





				PSTREAM_HEAD headCtx;

				status=FltGetStreamHandleContext(FltObjects->Instance,FltObjects->FileObject,(PFLT_CONTEXT *)&headCtx);
				if(!NT_SUCCESS(status))
				{
				//	KdPrint(("write 获取上下文失败  status = 08%x\n",status));
					if (NULL!= headCtx)
					{
						FltReleaseContext(headCtx);
					}

					return FLT_POSTOP_FINISHED_PROCESSING;
				} 

				if (NULL!= headCtx)
				{
					FltReleaseContext(headCtx);
				}

				STREAM_HEAD headStream ;
				headStream.isRead = FALSE;
				LARGE_INTEGER offset;
				offset.QuadPart = 0;
				ULONG readLen = 0;
				status = STATUS_SUCCESS;

				status = FltReadFile(FltObjects->Instance,
				FltObjects->FileObject,
				&offset,
				LENGTH_READ,
				(PVOID)headStream.fileHead,
				FLTFL_IO_OPERATION_PAGING,
				&readLen,
				NULL,
				NULL);

				//RtlStringCbCopyA(headStream.fileHead,40,"0");

				if (NT_SUCCESS(status))
				{
					//KdPrint(("head is %s ，读取的长度是  %ld ,%ld\n",headStream.fileHead,readLen,strlen(headStream.fileHead)));
					CHAR tmp[LENGTH_READ] = {0};
					CharToHex(headStream.fileHead,tmp);
					KdPrint(("原来的： %s,现在的：%s",headCtx->fileHead,tmp));

				}



























				/*strcat(STR,T);strcat(STR,";OP=4;C="); strcat(STR,procName);strcat(STR,";PID=");
				strcat(STR,PID);strcat(STR,";PPID="); strcat(STR,PPID);strcat(STR,";P=");
				strcat(STR,FileName);strcat(STR,";S="); strcat(STR,"1555\r\n");
				KdPrint(("%s",STR));*/


				strcat(STR,T);strcat(STR,";4;"); 
				strcat(STR,procName);strcat(STR,";");
				strcat(STR,PID);strcat(STR,";"); strcat(STR,PPID);strcat(STR,";");
				strcat(STR,FileName);strcat(STR,";"); strcat(STR,"1555\n");
































				PLOG_LIST pathListNode ,pathList;
				pathListNode = (PLOG_LIST)ExAllocatePool(NonPagedPool,sizeof(LOG_LIST));
				if (pathListNode == NULL)
				{
					KdPrint(("队列申请失败  \n"));  
				}
				//wcscpy(pathListNode->xxPath,pszDest);
				RtlCopyMemory(pathListNode->xxPath,STR,strlen(STR));
				InsertTailList(&HidePathListHeader,&pathListNode->listNode);//插入队尾



			}
			
					//KdPrint(("文件类型 = %d,文件路径:= %d ", temCtx.fileStyle.Length,temCtx.fileFullPath.Length));
					//KdPrint(("所在卷:= %d 父目录 = %d\n", temCtx.fileVolumeName.Length,temCtx.fileName.Length));
					//KdPrint(("文件路径:= %wZ , 文件类型 = %wZ", &temCtx.fileFullPath,&temCtx.fileStyle));	
					//KdPrint(("所在卷 =%wZ,父目录=%wZ \n ",&temCtx.fileVolumeName,&temCtx.fileName));	

		
	//		ULONG replyLength;  
	//		SCANNER_REPLY   Reply = {0};  
	//		replyLength = sizeof(SCANNER_REPLY);  

	//		PSCANNER_NOTIFICATION notification =(PSCANNER_NOTIFICATION) ExAllocatePool(NonPagedPool,sizeof(SCANNER_NOTIFICATION)); 
	////		if (notification == NULL)return ;  
	//		RtlZeroMemory(notification, sizeof(SCANNER_NOTIFICATION));  
	//		//notification->bCreate = Create;  
	//		RtlCopyMemory(notification->ProcessName, pName,strlen(pName)+1);

	//		status = FltSendMessage(gFilterHandle,   //句柄
	//			                    &gClientPort, //客户端端口
	//			                    notification,//发送缓冲
	//			                    sizeof(SCANNER_NOTIFICATION), //发送缓冲区的大小
	//								&Reply,
	//								&replyLength,
	//								NULL
	//								);

	//		if (NT_SUCCESS(status))  
	//			{  
	//			  KdPrint(("发送成功  %d\n", replyLength));  
	//			}  
	//		else  
	//			{  
	//				KdPrint(("发送失败  status = %08x\n",status));  
	//			}  



		   /* CHAR pszDest[30];
		    ULONG cbDest = 30;
			LPCSTR pszFormat = "%s %d + %d = %d.";
			CHAR* pszTxt = "The answer is";

			RtlStringCbPrintfA(pszDest, cbDest, pszFormat, pszTxt, 1, 2, "3\n");*/
		//	KdPrint(("%s",pszDest));




			



			//设置事件为有信号，通知  
			//KeSetEvent(g_pEventObject, 0, FALSE);  


			//if (pathListNode == NULL)
			//{
			//	return   FLT_POSTOP_FINISHED_PROCESSING;;
			//}
			//wcscpy(pathListNode->xxPath,L"你好！");
			//KeAcquireSpinLock(&HidePathListLock,&Irql);
			//InsertTailList(&HidePathListHeader,&pathListNode->listNode);
			//KeReleaseSpinLock(&HidePathListLock,Irql);

		} 




		//PSTREAM_HEAD headCtx;
		//status=FltGetStreamHandleContext(FltObjects->Instance,FltObjects->FileObject,(PFLT_CONTEXT *)&headCtx);
		//if(!NT_SUCCESS(status))
		//{
		//	KdPrint(("write 获取上下文流句柄失败 \n"));
		//	return FLT_POSTOP_FINISHED_PROCESSING;
		//} 


		//LARGE_INTEGER offset;
		//STREAM_HEAD headStream;
		//offset.QuadPart = 0;
		//ULONG readLen = 0;

		//status = FltReadFile(FltObjects->Instance,
		//	FltObjects->FileObject,
		//	&offset,
		//	LENGTH_READ,
		//	(PVOID)headStream.fileHead,
		//	FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET|FLTFL_IO_OPERATION_NON_CACHED,
		//	&readLen,
		//	NULL,
		//	NULL);
		//if (NT_SUCCESS(status))
		//{
		//	if (strncmp(headStream.fileHead,headCtx->fileHead,LENGTH_READ))
		//	{
		//		KdPrint(("相同\n"));
		//	}
		//}


		//if (NULL!=headCtx) //icedxu
		//{
		//	FltReleaseContext(headCtx);
		//}


	return FLT_POSTOP_FINISHED_PROCESSING;
}








////////////////////////////////SetInformation//////////////////////////////////
/****
 *在这个IRP中可判断重命名与删除操作
 **/
#pragma  LOCKEDCODE
FLT_PREOP_CALLBACK_STATUS
SetInformationPre(
					__inout PFLT_CALLBACK_DATA Data,
					__in PCFLT_RELATED_OBJECTS FltObjects,
					__deref_out_opt PVOID *CompletionContext
					)
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

	STREAM_HANDLE_CONTEXT temCtx;
	NTSTATUS status;

	//检查中断级
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
	{
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	//获取文件信息
	status = GetFileInformation(Data,FltObjects,&temCtx);
	if (NT_SUCCESS(status))
		{

			PCHAR procName=GetCurrentProcessName(ProcessNameOffset);
			PEPROCESS  p = FltGetRequestorProcess(Data);
			ULONG ProcessId = FltGetRequestorProcessId(Data);  
			ULONG ThreadId = (ULONG)PsGetThreadId(Data->Thread); 


			EnumProcess(ThreadId);
			//	KdPrint((" ThreadId = %u \n",ThreadId));
			


			if (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation)
			{
				if (!IsSecretProcess(procName))
				{
					//获取系统运行时间，此函数返回值已被处理只返回开机到到现在的秒数，可以放在日志的开头
					ULONG Time = GetTime();
					CHAR T[100]={0},PID[100]={0},PPID[100] ={0}  ;

					IntegerToChar(Time,T);
					IntegerToChar(ProcessId,PPID);
					IntegerToChar(ThreadId,PID);
					//KdPrint(("%s \n",T));
					//KdPrint(("PPID = %s,PID = %s \n",PPID,PID));

					CHAR FileName[260] ={0}; 

					NPUnicodeStringToChar(&temCtx.fileVolumeName, FileName,temCtx.fileVolumeName.Length);
					//KdPrint((" FileName = %s",FileName));
					//T=0;OP=1;C=test.exe;  PID=123;PPID=321;P=\\Device\\HarddiskVolume1\\test;S=15
					//CHAR STR[260] = {"T="};
					CHAR STR[260] = "XXX;";

					/*strcat(STR,T);strcat(STR,";OP=2;C="); strcat(STR,procName);strcat(STR,";PID=");
					strcat(STR,PID);strcat(STR,";PPID="); strcat(STR,PPID);strcat(STR,";P=");
					strcat(STR,FileName);strcat(STR,";S="); strcat(STR,"1555\r\n");
					KdPrint(("%s",STR));*/

					strcat(STR,T);strcat(STR,";2;"); 
					strcat(STR,procName);strcat(STR,";");
					strcat(STR,PID);strcat(STR,";"); strcat(STR,PPID);strcat(STR,";");
					strcat(STR,FileName);strcat(STR,";"); strcat(STR,"1555\n");

				

				   PLOG_LIST pathListNode ,pathList;
				   pathListNode = (PLOG_LIST)ExAllocatePool(NonPagedPool,sizeof(LOG_LIST));
				   if (pathListNode == NULL)
				   {
					   KdPrint(("队列初始化失败  \n"));  
				   }
				   RtlCopyMemory(pathListNode->xxPath,STR,strlen(STR));
				   InsertTailList(&HidePathListHeader,&pathListNode->listNode);//插入队尾

				   //设置事件为有信号，通知  
				 //  KeSetEvent(g_pEventObject, 0, FALSE); 

				}
		   }


			if (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileRenameInformation)
			{
				PCHAR procName=GetCurrentProcessName(ProcessNameOffset);
				if (!IsSecretProcess(procName))
				{
					//获取系统运行时间，此函数返回值已被处理只返回开机到到现在的秒数，可以放在日志的开头
					ULONG Time = GetTime();
					CHAR T[100]={0},PID[100]={0},PPID[100] ={0}  ;

					IntegerToChar(Time,T);
					IntegerToChar(ProcessId,PPID);
					IntegerToChar(ThreadId,PID);
					//KdPrint(("%s \n",T));
					//KdPrint(("PPID = %s,PID = %s \n",PPID,PID));

					CHAR FileName[260] ={'0'}; 

					NPUnicodeStringToChar(&temCtx.fileVolumeName, FileName,temCtx.fileVolumeName.Length);
					//KdPrint((" FileName = %s",FileName));
					//T=0;OP=1;C=test.exe;  PID=123;PPID=321;P=\\Device\\HarddiskVolume1\\test;S=15
				/*	CHAR STR[260] = {"T="};

					strcat(STR,T);strcat(STR,";OP=3;C="); strcat(STR,procName);strcat(STR,";PID=");
					strcat(STR,PID);strcat(STR,";PPID="); strcat(STR,PPID);strcat(STR,";P=");
					strcat(STR,FileName);strcat(STR,";S="); strcat(STR,"1555\r\n");
					KdPrint(("%s",STR));*/

					CHAR STR[260] = "XXX;";

					strcat(STR,T);strcat(STR,";3;"); 
					strcat(STR,procName);strcat(STR,";");
					strcat(STR,PID);strcat(STR,";"); strcat(STR,PPID);strcat(STR,";");
					strcat(STR,FileName);strcat(STR,";"); strcat(STR,"1555\n");

					PLOG_LIST pathListNode ,pathList;
					pathListNode = (PLOG_LIST)ExAllocatePool(NonPagedPool,sizeof(LOG_LIST));
					if (pathListNode == NULL)
					{
						KdPrint(("队列初始化失败  \n"));  
					}
					RtlCopyMemory(pathListNode->xxPath,STR,strlen(STR));
					InsertTailList(&HidePathListHeader,&pathListNode->listNode);//插入队尾
				}
			}

					/*KdPrint(("文件类型 = %d,文件路径:= %d ", temCtx.fileStyle.Length,temCtx.fileFullPath.Length));
					KdPrint(("所在卷:= %d 父目录 = %d\n", temCtx.fileVolumeName.Length,temCtx.fileName.Length));
					KdPrint(("文件路径:= %wZ , 文件类型 = %wZ", &temCtx.fileFullPath,&temCtx.fileStyle));	
					KdPrint(("所在卷 =%wZ,父目录=%wZ \n ",&temCtx.fileVolumeName,&temCtx.fileName));	*/

				
			
		} 

	return retValue;
}

#pragma  LOCKEDCODE
FLT_POSTOP_CALLBACK_STATUS
SetInformationPost(
					 __inout PFLT_CALLBACK_DATA Data,
					 __in PCFLT_RELATED_OBJECTS FltObjects,
					 __in PVOID CompletionContext,
					 __in FLT_POST_OPERATION_FLAGS Flags
					 )
{

	return FLT_POSTOP_FINISHED_PROCESSING;
}


///////////////////////////卸载函数/////////////////////////////////
#pragma  LOCKEDCODE
NTSTATUS
FilterUnload (
			  __in FLT_FILTER_UNLOAD_FLAGS Flags
			  )
{

	UNREFERENCED_PARAMETER( Flags );
	//FreeStrategy(key_word_header);	
	FltCloseCommunicationPort(serverPort);
	//EXIT = FALSE;
	PsSetCreateProcessNotifyRoutine(MyMiniFilterProcessNotify, TRUE);
	PsRemoveLoadImageNotifyRoutine(MyMiniFilterLoadImage);
	//ExDeleteNPagedLookasideList( &Pre2PostContextList );
	KdPrint(("即将卸载……\n"));
	FltUnregisterFilter( gFilterHandle );
	KdPrint(("完成卸载……\n"));
	return STATUS_SUCCESS;
}
















//////////ignore
#pragma  LOCKEDCODE
NTSTATUS
InstanceSetup (
			   __in PCFLT_RELATED_OBJECTS FltObjects,
			   __in FLT_INSTANCE_SETUP_FLAGS Flags,
			   __in DEVICE_TYPE VolumeDeviceType,
			   __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
			   )
{
	UNREFERENCED_PARAMETER( Flags );
	UNREFERENCED_PARAMETER( VolumeDeviceType );
	UNREFERENCED_PARAMETER( VolumeFilesystemType );
	////KdPrind(("InstanceSetup\n"));
	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
#pragma  LOCKEDCODE
VOID 
CleanupStreamHandleContext(
						   __in PFLT_CONTEXT Context,
						   __in FLT_CONTEXT_TYPE ContextType				 
						   )
{

	UNREFERENCED_PARAMETER( ContextType );
	//KdPrint(("进入CleanupStreamHandleContext \n"));

	PSTREAM_HEAD streamHandleContext;

	switch(ContextType)
	{
	case  FLT_STREAMHANDLE_CONTEXT:
		{
			//streamHandleContext = (PSTREAM_HEAD) Context;
			//KdPrint(("进入FLT_STREAMHANDLE_CONTEXT \n"));
			
			break;

		}
	}

	//KdPrint(("CleanupStreamHandleContext离开\n"));
}

//////////////////////////////////////////////////////////////////////////

NTSTATUS
InstanceQueryTeardown (
					   __in PCFLT_RELATED_OBJECTS FltObjects,
					   __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
					   )

{

	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( Flags );
	//检查中断级
	PAGED_CODE();	
	return STATUS_SUCCESS;
}

/************************************************************************/
/*                    通信口回调函数                                    */
/************************************************************************/

//连接回调
NTSTATUS
MyConnectionCallback(
					 __in PFLT_PORT ClientPort,
					 __in_opt PVOID ServerPortCookie,
					 __in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
					 __in ULONG SizeOfContext,
					 __deref_out_opt PVOID *ConnectionPortCookie
					 )
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER( ServerPortCookie );
	UNREFERENCED_PARAMETER( ConnectionContext );
	UNREFERENCED_PARAMETER( SizeOfContext);
	UNREFERENCED_PARAMETER( ConnectionPortCookie );
	ASSERT(gClientPort == NULL);
	//KdPrind(("Connect\n"));
	gClientPort = ClientPort; //保存供以后使用
	return STATUS_SUCCESS;
}


//关闭回调
VOID
MyDisconnectCallback (
					  __in_opt PVOID ConnectionCookie
					  )
{
	//KdPrind(("MyDisconnectCallback"));
	PAGED_CODE();
	UNREFERENCED_PARAMETER(ConnectionCookie);
	//KdPrind(("Disconnect\n"));
	//关闭通信连接
	FltCloseClientPort(gFilterHandle , &gClientPort);
}

#pragma  LOCKEDCODE
//消息回调
NTSTATUS
MyMessageCallback (
				   __in_opt PVOID PortCookie,
				   __in_bcount_opt(InputBufferLength) PVOID InputBuffer,
				   __in ULONG InputBufferLength,
				   __out_bcount_part_opt(OutputBufferLength,*ReturnOutputBufferLength) PVOID OutputBuffer,
				   __in ULONG OutputBufferLength,
				   __out PULONG ReturnOutputBufferLength
				   )
{
	
	 PAGED_CODE();

	 UNREFERENCED_PARAMETER( PortCookie );
	 UNREFERENCED_PARAMETER( OutputBufferLength );
	 UNREFERENCED_PARAMETER(InputBuffer);
	 UNREFERENCED_PARAMETER(InputBufferLength);

	  WCHAR *p;
	 __try{
		  
		 p = (PWCHAR)InputBuffer;
		 if (InputBuffer != NULL)
		 {
			 //KdPrint(("用户发来的信息是： %S\n",p));
			 KdPrint(("用户发来的信息是： %S \n",InputBuffer));
			 KdPrint(("InputBufferLength = %d \n",InputBufferLength));
		 }	
	 }
	 __except(EXCEPTION_EXECUTE_HANDLER){
        KdPrint(("%s \n",p));
   }
	return STATUS_SUCCESS;

}



VOID 
	MyMiniFilterLoadImage( __in_opt PUNICODE_STRING FullImageName, __in HANDLE ProcessId, __in PIMAGE_INFO ImageInfo )
{
	UNREFERENCED_PARAMETER(ImageInfo);

	if (FullImageName)
	{
		//DbgPrint("MyMiniFilterLoadImage, image name: %wZ, pid: %d\n", FullImageName, ProcessId);
	}
//	else
		//DbgPrint("MyMiniFilterLoadImage, image name: null, pid: %d\n", ProcessId);
}




VOID
	MyMiniFilterProcessNotify(
	IN HANDLE  ParentId,
	IN HANDLE  ProcessId,
	IN BOOLEAN  Create
	)
{
	//DbgPrint("MyMiniFilterProcessNotify, pid: %d, tid: %d, create: %d\n", ParentId, ProcessId, Create);
}

//枚举指定进程的线程
VOID EnumThread(PEPROCESS Process)
{
	ULONG i = 0, c = 0;
	PETHREAD ethrd = NULL;
	PEPROCESS eproc = NULL;
	for (i = 4; i<262144; i = i + 4)
	{
		ethrd = LookupThread((HANDLE)i);
		if (ethrd != NULL)
		{
			//获得线程所属进程
			eproc = IoThreadToProcess(ethrd);
			if (eproc == Process)
			{
				//打印出 ETHREAD 和 TID
				DbgPrint("ETHREAD=%p, TID=%ld\n",
					ethrd,
					(ULONG)PsGetThreadId(ethrd));
			}
			ObDereferenceObject(ethrd);
		}
	}
}


//根据线程 ID 返回线程 ETHREAD，失败返回 NULL
PETHREAD LookupThread(HANDLE Tid)
{
	PETHREAD ethread;
	if (NT_SUCCESS(PsLookupThreadByThreadId(Tid, &ethread)))
		return ethread;
	else
		return NULL;
}