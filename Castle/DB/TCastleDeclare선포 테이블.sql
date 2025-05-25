DROP Table [TCastleDeclareList]
go
DROP PROCEDURE [gp_Castle_Add_DeclareList]
go
DROP PROCEDURE [gp_Castle_Del_DeclareList]
go
DROP PROCEDURE [gp_Castle_Load_DeclareList]
go

CREATE TABLE [dbo].[TCastleDeclareList](
	[castleIndex] [int] NOT NULL,
	[mastername] [nvarchar](20) NULL,
	[clanname] [nvarchar](20) NULL,
	[clanunique] [int] NOT NULL,
	[simbolIndex1] [int] NOT NULL,
	[simbolIndex2] [int] NOT NULL,
	[simbolIndex3] [int] NOT NULL,
	[simbolurl] nvarchar(21) NULL,
	[money] [bigint] NOT NULL,
	[memberCount] [int] NOT NULL,
	[datetime] [datetime] NOT NULL

) ON [PRIMARY]
GO
ALTER TABLE [dbo].[TCastleDeclareList] ADD  CONSTRAINT [DF_TCastleDeclareList_mastername]  DEFAULT ('') FOR [mastername]
GO
ALTER TABLE [dbo].[TCastleDeclareList] ADD  CONSTRAINT [DF_TCastleDeclareList_clanname]  DEFAULT ('') FOR [clanname]
GO
ALTER TABLE [dbo].[TCastleDeclareList] ADD  CONSTRAINT [DF_TCastleDeclareList_datetime]  DEFAULT (getdate()) FOR [datetime]
GO
ALTER TABLE [dbo].[TCastleDeclareList] ADD  CONSTRAINT [DF_TCastleDeclareList_simbolurl]  DEFAULT (N'') FOR [simbolurl]
GO

CREATE PROCEDURE [dbo].[gp_Castle_Add_DeclareList]

/*1*/@castleIdx int,
/*2*/@clanunique int,
/*3*/@simbolindex1 int, 
/*4*/@simbolindex2 int, 
/*5*/@simbolindex3 int, 
/*6*/@simbolurl nvarChar(20),
/*7*/@declareMoney bigint,
/*8*/@membercnt int,
/*9*/@mastername nvarchar(20),
/*10*/@clanname nvarchar(20)

AS
DECLARE @rowCount int =0
SELECT @rowCount = COUNT(*) FROM TCastleDeclareList WHERE castleIndex = @castleIdx and clanunique = @clanunique
IF(@rowCount <> 0)
	RETURN 1
BEGIN
	INSERT INTO [TCastleDeclareList]([castleIndex],	[mastername] ,	[clanname],	[clanunique],[simbolIndex1] ,[simbolIndex2] ,[simbolIndex3],[simbolurl],[money] ,[memberCount] )
	VALUES(@castleIdx,@mastername,@clanname,@clanunique,@simbolindex1,@simbolindex2,@simbolindex3,@simbolurl,@declareMoney,@membercnt)
	IF(@@ERROR <> 0)
		RETURN 1
	RETURN 0
END


GO

CREATE PROCEDURE [dbo].[gp_Castle_Del_DeclareList]

@type int,
@castleIdx int,
@clanunique int
AS

DECLARE @rowCount int =0
DECLARE @rowdate datetime = getdate()

IF(@type = 3)
BEGIN
	DELETE FROM TCastleDeclareList where castleIndex = @castleIdx
	RETURN 0
END
SELECT @rowCount = 1, @rowdate = [datetime] FROM TCastleDeclareList WHERE castleIndex = @castleIdx and [clanunique] = @clanunique
IF(@rowCount = 0)
	RETURN 1

--IF(@type = 1 AND DATEADD(MINUTE,-5,getdate()) < @rowdate) -- 5분 이내에 들어간 row라면 실패. 
--	RETURN 2

DELETE FROM TCastleDeclareList where castleIndex = @castleIdx and [clanunique] = @clanunique
IF(@@ERROR <> 0)
	RETURN 3
RETURN 0

go

CREATE PROCEDURE [dbo].[gp_Castle_Load_DeclareList]
@castleIdx int


AS
BEGIN
	select 
	[clanunique] ,
	[simbolIndex1] ,
	[simbolIndex2] ,
	[simbolIndex3] ,
	[simbolurl],
	[money] ,
	[memberCount] ,
	[mastername] ,
	[clanname] 

	from TCastleDeclareList where castleIndex = @castleIdx 
	
END


